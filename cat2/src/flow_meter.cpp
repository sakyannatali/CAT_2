#include "flow_meter.h"
#include "config.h"
#include "app_state.h"
#include "pure_logic.h"

struct IsrFlow { volatile uint32_t pulses,lastPulseUs,previousPulseUs,lastAcceptedUs; };
static IsrFlow isrFlow[FLOW_SENSOR_COUNT];
static uint32_t previousCount[FLOW_SENSOR_COUNT], previousUpdateUs[FLOW_SENSOR_COUNT];
static float ema[FLOW_SENSOR_COUNT];
static bool tableValid[FLOW_SENSOR_COUNT];

static void onPulse(uint8_t index) { const uint32_t now=micros(); IsrFlow &f=isrFlow[index]; if((uint32_t)(now-f.lastAcceptedUs)<FLOW_MIN_PULSE_PERIOD_US)return; f.previousPulseUs=f.lastPulseUs; f.lastPulseUs=now; f.lastAcceptedUs=now; ++f.pulses; }
static void onPulse0(){onPulse(0);} static void onPulse1(){onPulse(1);}
static const CalibrationPoint *tableFor(uint8_t i, uint8_t &count) { count=i==0?FLOW1_CALIBRATION_COUNT:FLOW2_CALIBRATION_COUNT; return i==0?FLOW1_CALIBRATION:FLOW2_CALIBRATION; }
bool flowMeterIsCalibrated(){ return FLOW_CALIBRATED && tableValid[0] && tableValid[1]; }
void flowMeterBegin(){
  for(uint8_t i=0;i<FLOW_SENSOR_COUNT;++i){ memset(&isrFlow[i],0,sizeof(IsrFlow)); pinMode(FLOW_PINS[i],INPUT); const int irq=digitalPinToInterrupt(FLOW_PINS[i]); if(irq==NOT_AN_INTERRUPT){Serial.print(F("FLOW ERROR: pin has no interrupt: "));Serial.println(FLOW_PINS[i]); continue;} attachInterrupt(irq,i==0?onPulse0:onPulse1,FLOW_INTERRUPT_MODE); uint8_t n; tableValid[i]=calibrationTableValid(tableFor(i,n),n); app.flow[i].calibrated=FLOW_CALIBRATED&&tableValid[i]; previousUpdateUs[i]=micros(); }
  Serial.print(F("Flow calibration: ")); Serial.println(flowMeterIsCalibrated()?F("valid"):F("disabled or invalid table"));
}
void flowMeterService(uint32_t now){
  (void)now;
  for(uint8_t i=0;i<FLOW_SENSOR_COUNT;++i){
    const uint32_t nowUs=micros(); if((uint32_t)(nowUs-previousUpdateUs[i])<FLOW_UPDATE_INTERVAL_MS*1000UL)continue;
    uint32_t count,last,previous; noInterrupts(); count=isrFlow[i].pulses; last=isrFlow[i].lastPulseUs; previous=isrFlow[i].previousPulseUs; interrupts();
    const uint32_t windowUs=nowUs-previousUpdateUs[i], delta=count-previousCount[i]; previousUpdateUs[i]=nowUs; previousCount[i]=count;
    const uint32_t age=last?(uint32_t)((nowUs-last)/1000UL):UINT32_MAX; const uint32_t period=(last&&previous)?last-previous:0;
    FlowState &s=app.flow[i]; s.totalPulses=count; s.lastPulsePeriodUs=period; s.lastPulseAgeMs=age; s.stale=age>FLOW_ZERO_TIMEOUT_MS; s.valid=last && !s.stale; s.frequencyHz=hybridFrequency(delta,windowUs,period,age,FLOW_ZERO_TIMEOUT_MS);
    s.calibrated=FLOW_CALIBRATED&&tableValid[i];
    if(s.calibrated && s.valid) { uint8_t n; s.massFlowKgH=interpolateCalibration(tableFor(i,n),n,s.frequencyHz); const float alpha=(float)FLOW_UPDATE_INTERVAL_MS/(FLOW_FILTER_TIME_MS+FLOW_UPDATE_INTERVAL_MS); ema[i]+=alpha*(s.massFlowKgH-ema[i]); s.filteredMassFlowKgH=ema[i]; }
    else { s.massFlowKgH=0; s.filteredMassFlowKgH=0; }
  }
}
void flowMeterReset(){ noInterrupts(); for(uint8_t i=0;i<FLOW_SENSOR_COUNT;++i)memset(&isrFlow[i],0,sizeof(IsrFlow)); interrupts(); for(uint8_t i=0;i<FLOW_SENSOR_COUNT;++i){previousCount[i]=0;ema[i]=0;app.flow[i].totalPulses=0;} Serial.println(F("Flow pulse counters reset.")); }
void flowMeterPrintStatus(bool raw){ for(uint8_t i=0;i<FLOW_SENSOR_COUNT;++i){ const FlowState&s=app.flow[i]; Serial.print(F("flow "));Serial.print(i+1);Serial.print(F(": pulses="));Serial.print(s.totalPulses);Serial.print(F(" hz="));Serial.print(s.frequencyHz,3);Serial.print(F(" period_us="));Serial.print(s.lastPulsePeriodUs);Serial.print(F(" age_ms="));Serial.print(s.lastPulseAgeMs);Serial.print(F(" calibrated="));Serial.print(s.calibrated?F("yes"):F("no"));Serial.print(F(" stale="));Serial.print(s.stale?F("yes"):F("no"));if(s.calibrated){Serial.print(F(" kg/h="));Serial.print(s.massFlowKgH,2);Serial.print(F(" filtered="));Serial.print(s.filteredMassFlowKgH,2);}if(raw){Serial.print(F(" valid="));Serial.print(s.valid?F("yes"):F("no"));}Serial.println();} }
