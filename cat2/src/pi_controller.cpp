#include "pi_controller.h"
#include "app_state.h"
#include "flow_meter.h"
#include "config.h"
#include "pure_logic.h"

static PiControllerCore pi;
static TimedMovingAverage average;
static uint32_t lastUpdate=0;
void piControllerBegin(){pi.kp=PI_DEFAULT_KP;pi.tiSeconds=PI_DEFAULT_TI_S;pi.reset(0);}
static bool feedback(float &raw,float &filtered){const FlowFeedbackSource src=getFlowFeedbackSource();const FlowState&a=app.flow[0],&b=app.flow[1];if(src==FLOW_SENSOR_1){if(!a.valid)return false;raw=a.massFlowKgH;filtered=a.filteredMassFlowKgH;return true;}if(src==FLOW_SENSOR_2){if(!b.valid)return false;raw=b.massFlowKgH;filtered=b.filteredMassFlowKgH;return true;}uint8_t n=0;raw=filtered=0;if(a.valid){raw+=a.massFlowKgH;filtered+=a.filteredMassFlowKgH;++n;}if(b.valid){raw+=b.massFlowKgH;filtered+=b.filteredMassFlowKgH;++n;}if(!n)return false;raw/=n;filtered/=n;return true;}
void piControllerService(uint32_t now){if((uint32_t)(now-lastUpdate)<PI_UPDATE_INTERVAL_MS)return;float dt=lastUpdate?(float)(now-lastUpdate)/1000.0f:1.0f;lastUpdate=now;float raw,filtered;bool fresh=flowMeterIsCalibrated()&&feedback(raw,filtered);if(fresh)average.add(filtered,now);uint8_t n=0;float avg=average.value(now,10000UL,&n);if(app.flowControlMode!=FLOW_AUTO)return;if(!flowMeterIsCalibrated()){app.autoBlocked=true;app.autoBlockReason="FLOW NOT CALIBRATED";setFlowControlMode(FLOW_MANUAL);return;}if(!app.ventRelayOn){app.autoBlocked=true;app.autoBlockReason="FAN OFF";return;}if(!fresh||n==0||isnan(avg)){app.autoBlocked=true;app.autoBlockReason="FLOW SENSOR FAULT";setFlowControlMode(FLOW_MANUAL);return;}app.autoBlocked=false;app.autoBlockReason="";const float e=(app.flowSetpointKgH-avg)/PI_FLOW_FULL_SCALE_KG_H;PiTerms terms=pi.update(e,dt,PI_MAX_OUTPUT_STEP_PER_S*dt,PI_INTEGRAL_LIMIT);app.piError=e;app.piP=terms.p;app.piI=terms.i;app.piOutputPercent=terms.requested;setFanPowerPercent(terms.requested);}
void piControllerSetKp(float value){if(value>=0&&value<=20)pi.kp=value;}
void piControllerSetTi(float seconds){if(seconds>=1&&seconds<=3600)pi.tiSeconds=seconds;}
float piControllerKp(){return pi.kp;} float piControllerTi(){return pi.tiSeconds;}
void piControllerPrepareAuto(float currentOutput){pi.makeBumpless(0.0f,currentOutput);app.piOutputPercent=currentOutput;}
void piControllerPrintStatus(){float raw,filtered;bool ok=feedback(raw,filtered);uint8_t n;float avg=average.value(millis(),10000,&n);Serial.print(F("control mode="));Serial.print(app.flowControlMode==FLOW_AUTO?F("AUTO"):F("MANUAL"));Serial.print(F(" setpoint="));Serial.print(app.flowSetpointKgH,2);Serial.print(F(" raw="));if(ok)Serial.print(raw,2);else Serial.print(F("ERR"));Serial.print(F(" filtered10s="));if(n)Serial.print(avg,2);else Serial.print(F("ERR"));Serial.print(F(" error="));Serial.print(app.piError,3);Serial.print(F(" P="));Serial.print(app.piP,2);Serial.print(F(" I="));Serial.print(app.piI,2);Serial.print(F(" requested="));Serial.print(app.requestedFanPowerPercent,1);Serial.print(F(" applied="));Serial.print(app.appliedFanPowerPercent,1);if(app.autoBlocked){Serial.print(F(" blocked="));Serial.print(app.autoBlockReason);}Serial.println();}
