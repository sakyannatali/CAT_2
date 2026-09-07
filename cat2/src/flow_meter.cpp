#include "flow_meter.h"
#include "config.h"
#include "app_state.h"
#include "pure_logic.h"
#include "generated/air_density_table.h"

struct IsrFlow { volatile uint32_t pulses, lastPulseUs, previousPulseUs, lastAcceptedUs; };
struct FlowRuntime {
  TimedMovingAverage<12> filter;
  uint32_t previousCount, previousUpdateUs, lastFilterSampleMs, lastDisplayUpdateMs, startedMs;
  bool filterSampled, displayInitialized;
};

static IsrFlow isrFlow[FLOW_SENSOR_COUNT];
static FlowRuntime runtime[FLOW_SENSOR_COUNT];
static float lastDensityCorrection=NAN, lastDensityToutC=NAN;
static uint32_t lastDensityMs=0;
static bool hasLastDensity=false, densityOutOfRange=false;

static void onPulse(uint8_t index) {
  const uint32_t now=micros();
  IsrFlow &f=isrFlow[index];
  if ((uint32_t)(now-f.lastAcceptedUs)<FLOW_MIN_PULSE_PERIOD_US) return;
  f.previousPulseUs=f.lastPulseUs;
  f.lastPulseUs=now;
  f.lastAcceptedUs=now;
  ++f.pulses;
}
static void onPulse0(){onPulse(0);} static void onPulse1(){onPulse(1);}

bool flowMeterConversionConfigured() { return FLOW_CONVERSION_CONFIGURED; }

static bool densityForNow(uint32_t now, float &correction, float &tout) {
  densityOutOfRange=false;
  const ValueState &outlet=app.outletTemperature;
  if (outlet.valid && (uint32_t)(now-outlet.updatedMs)<=TEMPERATURE_STALE_MS) {
    tout=outlet.value;
    if (!airDensityCorrectionAt(tout, correction)) { densityOutOfRange=true; return false; }
    lastDensityCorrection=correction;
    lastDensityToutC=tout;
    lastDensityMs=now;
    hasLastDensity=true;
    return true;
  }
  if (hasLastDensity && (uint32_t)(now-lastDensityMs)<=FLOW_OUTLET_TEMPERATURE_HOLD_MS) {
    correction=lastDensityCorrection;
    tout=lastDensityToutC;
    return true;
  }
  correction=NAN;
  tout=NAN;
  return false;
}

void flowMeterBegin() {
  const uint32_t now=millis();
  for (uint8_t i=0;i<FLOW_SENSOR_COUNT;++i) {
    memset(&isrFlow[i],0,sizeof(IsrFlow));
    runtime[i]=FlowRuntime();
    runtime[i].previousUpdateUs=micros();
    runtime[i].startedMs=now;
    pinMode(FLOW_PINS[i],INPUT);
    const int irq=digitalPinToInterrupt(FLOW_PINS[i]);
    if (irq==NOT_AN_INTERRUPT) {
      Serial.print(F("FLOW ERROR: pin has no interrupt: "));
      Serial.println(FLOW_PINS[i]);
      continue;
    }
    attachInterrupt(irq,i==0?onPulse0:onPulse1,FLOW_INTERRUPT_MODE);
    app.flow[i].conversionConfigured=FLOW_CONVERSION_CONFIGURED;
  }
  Serial.println(F("Flow conversion: configured (L/min, density-corrected)"));
}

void flowMeterService(uint32_t now) {
  float correction, tout;
  const bool densityValid=densityForNow(now,correction,tout);
  for (uint8_t i=0;i<FLOW_SENSOR_COUNT;++i) {
    const uint32_t nowUs=micros();
    if ((uint32_t)(nowUs-runtime[i].previousUpdateUs)<FLOW_UPDATE_INTERVAL_MS*1000UL) continue;

    uint32_t count,last,previous;
    noInterrupts();
    count=isrFlow[i].pulses;
    last=isrFlow[i].lastPulseUs;
    previous=isrFlow[i].previousPulseUs;
    interrupts();

    const uint32_t windowUs=nowUs-runtime[i].previousUpdateUs;
    const uint32_t delta=count-runtime[i].previousCount;
    runtime[i].previousUpdateUs=nowUs;
    runtime[i].previousCount=count;
    const uint32_t age=last ? (uint32_t)((nowUs-last)/1000UL) : UINT32_MAX;
    const uint32_t period=(last&&previous) ? last-previous : 0;
    const bool frequencyValid=last&&previous&&age<=FLOW_ZERO_TIMEOUT_MS;
    const bool zeroFlow=last ? age>FLOW_ZERO_TIMEOUT_MS : (uint32_t)(now-runtime[i].startedMs)>=FLOW_ZERO_TIMEOUT_MS;

    FlowState &s=app.flow[i];
    s.totalPulses=count;
    s.lastPulsePeriodUs=period;
    s.lastPulseAgeMs=age;
    s.frequencyHz=hybridFrequency(delta,windowUs,period,age,FLOW_ZERO_TIMEOUT_MS);
    s.zeroFlow=zeroFlow;
    s.stale=!frequencyValid&&!zeroFlow;
    s.conversionConfigured=FLOW_CONVERSION_CONFIGURED;
    s.densityValid=densityValid;
    s.densityCorrection=densityValid ? correction : NAN;
    s.outletTemperatureUsedC=densityValid ? tout : NAN;
    s.valid=densityValid && (frequencyValid||zeroFlow);
    s.instantFlowLpm=s.valid ? correctedFlowLpm(i,s.frequencyHz,correction,zeroFlow) : NAN;

    if (s.valid && (!runtime[i].filterSampled || (uint32_t)(now-runtime[i].lastFilterSampleMs)>=FLOW_FILTER_SAMPLE_INTERVAL_MS)) {
      runtime[i].filter.add(s.instantFlowLpm,now);
      runtime[i].lastFilterSampleMs=now;
      runtime[i].filterSampled=true;
    }
    uint8_t sampleCount=0;
    const float filtered=runtime[i].filter.value(now,FLOW_FILTER_WINDOW_MS,&sampleCount);
    s.filteredFlowLpm=sampleCount ? filtered : NAN;
    if (s.valid && sampleCount && (!runtime[i].displayInitialized || (uint32_t)(now-runtime[i].lastDisplayUpdateMs)>=FLOW_DISPLAY_UPDATE_MS)) {
      s.displayFlowLpm=s.filteredFlowLpm;
      runtime[i].lastDisplayUpdateMs=now;
      runtime[i].displayInitialized=true;
    }
  }
}

const char *flowMeterAutoBlockReason() {
  const FlowFeedbackSource source=getFlowFeedbackSource();
  const FlowState &a=app.flow[0], &b=app.flow[1];
  if ((source==FLOW_SENSOR_1 && !a.densityValid) ||
      (source==FLOW_SENSOR_2 && !b.densityValid) ||
      (source==FLOW_AVERAGE_OF_VALID && !a.densityValid && !b.densityValid))
    return densityOutOfRange ? "OUTLET TEMP OUT OF RANGE" : "OUTLET TEMP INVALID";
  return "FLOW SENSOR FAULT";
}

void flowMeterReset() {
  noInterrupts();
  for(uint8_t i=0;i<FLOW_SENSOR_COUNT;++i) memset(&isrFlow[i],0,sizeof(IsrFlow));
  interrupts();
  const uint32_t now=millis();
  for(uint8_t i=0;i<FLOW_SENSOR_COUNT;++i) {
    runtime[i]=FlowRuntime();
    runtime[i].previousUpdateUs=micros();
    runtime[i].startedMs=now;
    app.flow[i].totalPulses=0;
    app.flow[i].frequencyHz=0;
    app.flow[i].instantFlowLpm=NAN;
    app.flow[i].filteredFlowLpm=NAN;
    app.flow[i].displayFlowLpm=NAN;
  }
  Serial.println(F("Flow pulse counters and filters reset."));
}

void flowMeterPrintStatus(bool raw) {
  for(uint8_t i=0;i<FLOW_SENSOR_COUNT;++i) {
    const FlowState&s=app.flow[i];
    Serial.print(F("Flow")); Serial.print(i+1); Serial.println(F(":"));
    Serial.print(F("  pulses: ")); Serial.println(s.totalPulses);
    Serial.print(F("  frequency: ")); Serial.print(s.frequencyHz,2); Serial.println(F(" Hz"));
    Serial.print(F("  period_us: ")); Serial.println(s.lastPulsePeriodUs);
    Serial.print(F("  age_ms: ")); Serial.println(s.lastPulseAgeMs);
    Serial.print(F("  instant: ")); if(s.valid)Serial.print(s.instantFlowLpm,1);else Serial.print(F("ERR")); Serial.println(F(" L/min"));
    Serial.print(F("  filtered5s: ")); if(s.valid&&!isnan(s.filteredFlowLpm))Serial.print(s.filteredFlowLpm,1);else Serial.print(F("ERR")); Serial.println(F(" L/min"));
    Serial.print(F("  display: ")); if(s.valid&&!isnan(s.displayFlowLpm))Serial.print(s.displayFlowLpm,1);else Serial.print(F("ERR")); Serial.println(F(" L/min"));
    Serial.print(F("  Tout: ")); if(s.densityValid)Serial.print(s.outletTemperatureUsedC,1);else Serial.print(F("ERR")); Serial.println(F(" C"));
    Serial.print(F("  densityCorrection: ")); if(s.densityValid)Serial.println(s.densityCorrection,6);else Serial.println(F("ERR"));
    Serial.print(F("  zero_flow: ")); Serial.print(s.zeroFlow?F("yes"):F("no"));
    Serial.print(F(" valid: ")); Serial.print(s.valid?F("yes"):F("no"));
    Serial.print(F(" stale: ")); Serial.println(s.stale?F("yes"):F("no"));
    if(raw) { Serial.print(F("  conversion: ")); Serial.println(s.conversionConfigured?F("configured"):F("missing")); }
  }
}

void flowMeterPrintDensity() {
  float correction,tout,rhoOut;
  const bool valid=densityForNow(millis(),correction,tout);
  Serial.print(F("rho24: ")); Serial.print(AIR_DENSITY_RHO24_KG_M3,6); Serial.println(F(" kg/m3"));
  Serial.print(F("Tout: ")); if(valid)Serial.print(tout,2);else Serial.print(F("ERR")); Serial.println(F(" C"));
  Serial.print(F("rhoOut: ")); if(valid&&airDensityRhoOutAt(tout,rhoOut))Serial.print(rhoOut,6);else Serial.print(F("ERR")); Serial.println(F(" kg/m3"));
  Serial.print(F("densityCorrection: ")); if(valid)Serial.println(correction,6);else Serial.println(F("ERR"));
  Serial.print(F("valid: ")); Serial.println(valid?F("yes"):F("no"));
}
