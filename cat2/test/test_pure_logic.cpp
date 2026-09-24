#include <unity.h>
#include <cstring>
#include <math.h>
#include "config.h"
#include "pure_logic.h"
#include "flow_model.h"

// Native tests intentionally compile the pure model implementation without the
// Arduino-only control modules.
#include "../src/flow_model.cpp"

void setUp() {} void tearDown() {}

static float saturation(float a,float k,float power) {
  return FLOW_MODEL_LPM_PER_MPS*a*(1.0f-expf(-k*power));
}

void test_model_direct_reference_curve() {
  const FlowEstimate value=estimateFlow(30.0f,-100);
  TEST_ASSERT_FLOAT_WITHIN(0.001f,saturation(37.4715298098f,0.031191095466f,30.0f),value.output1Lpm);
  TEST_ASSERT_FLOAT_WITHIN(0.001f,saturation(7.2859015379f,0.027189607669f,30.0f),value.output2Lpm);
  TEST_ASSERT_FLOAT_WITHIN(0.001f,value.output1Lpm+value.output2Lpm,value.totalLpm);
}
void test_model_zero_and_nonnegative() {
  const FlowEstimate zero=estimateFlow(0.0f,0);
  const FlowEstimate negativePower=estimateFlow(-20.0f,100);
  TEST_ASSERT_EQUAL_FLOAT(0.0f,zero.output1Lpm); TEST_ASSERT_EQUAL_FLOAT(0.0f,zero.output2Lpm); TEST_ASSERT_EQUAL_FLOAT(0.0f,zero.totalLpm);
  TEST_ASSERT_EQUAL_FLOAT(0.0f,negativePower.totalLpm);
  TEST_ASSERT_TRUE(estimateFlow(100.0f,-100).totalLpm>0.0f);
}
void test_negative_gate_interpolation() {
  const float p=40.0f;
  const FlowEstimate minus50=estimateFlow(p,-50), zero=estimateFlow(p,0), minus30=estimateFlow(p,-30);
  TEST_ASSERT_FLOAT_WITHIN(0.001f,0.6f*minus50.output1Lpm+0.4f*zero.output1Lpm,minus30.output1Lpm);
  TEST_ASSERT_FLOAT_WITHIN(0.001f,0.6f*minus50.output2Lpm+0.4f*zero.output2Lpm,minus30.output2Lpm);
}
void test_positive_gate_derived_geometry() {
  const float p=40.0f;
  const FlowEstimate minus100=estimateFlow(p,-100), minus50=estimateFlow(p,-50), minus30=estimateFlow(p,-30);
  const FlowEstimate plus50=estimateFlow(p,50), plus100=estimateFlow(p,100);
  TEST_ASSERT_FLOAT_WITHIN(0.001f,minus30.output2Lpm,plus50.output1Lpm);
  TEST_ASSERT_FLOAT_WITHIN(0.001f,minus30.output1Lpm,plus50.output2Lpm);
  TEST_ASSERT_FLOAT_WITHIN(0.001f,minus100.output2Lpm,plus100.output1Lpm);
  TEST_ASSERT_FLOAT_WITHIN(0.001f,minus100.output1Lpm,plus100.output2Lpm);
  TEST_ASSERT_TRUE(fabsf(plus50.output1Lpm-minus50.output1Lpm)>0.01f);
  TEST_ASSERT_FLOAT_WITHIN(0.001f,plus50.output1Lpm+plus50.output2Lpm,plus50.totalLpm);
}
void test_model_inverse_and_status() {
  const FlowModelSolveResult normal=solveFlowModel(estimateFlow(35.0f,-30).totalLpm,-30);
  TEST_ASSERT_FLOAT_WITHIN(0.03f,35.0f,normal.fanPowerPercent); TEST_ASSERT_EQUAL(FLOW_MODEL_OK,normal.status);
  const FlowModelSolveResult below=solveFlowModel(estimateFlow(4.0f,0).totalLpm,0);
  TEST_ASSERT_FLOAT_WITHIN(0.03f,4.0f,below.fanPowerPercent); TEST_ASSERT_EQUAL(FLOW_MODEL_BELOW_CALIBRATED_RANGE,below.status);
  const FlowModelSolveResult extrapolated=solveFlowModel(estimateFlow(80.0f,50).totalLpm,50);
  TEST_ASSERT_FLOAT_WITHIN(0.03f,80.0f,extrapolated.fanPowerPercent); TEST_ASSERT_EQUAL(FLOW_MODEL_EXTRAPOLATED,extrapolated.status);
  const FlowModelSolveResult impossible=solveFlowModel(estimateFlow(100.0f,0).totalLpm+1.0f,0);
  TEST_ASSERT_EQUAL_FLOAT(100.0f,impossible.fanPowerPercent); TEST_ASSERT_EQUAL(FLOW_MODEL_UNREACHABLE,impossible.status);
  TEST_ASSERT_EQUAL(FLOW_MODEL_NONE,solveFlowModel(0.0f,0).status);
}
void test_model_inverse_for_500_lpm_and_unreachable_status() {
  const int16_t gates[]={-100,-30,0,50,100};
  for(uint8_t i=0;i<sizeof(gates)/sizeof(gates[0]);++i) {
    const float target=500.0f;
    const FlowModelSolveResult result=solveFlowModel(target,gates[i]);
    TEST_ASSERT_TRUE(result.fanPowerPercent>=0.0f && result.fanPowerPercent<=100.0f);
    const FlowEstimate estimate=estimateFlow(result.fanPowerPercent,gates[i]);
    const float maximum=estimateFlow(100.0f,gates[i]).totalLpm;
    if(maximum<target) {
      TEST_ASSERT_EQUAL(FLOW_MODEL_UNREACHABLE,result.status);
      TEST_ASSERT_EQUAL_FLOAT(100.0f,result.fanPowerPercent);
      TEST_ASSERT_TRUE(estimate.totalLpm<target);
    } else {
      TEST_ASSERT_NOT_EQUAL(FLOW_MODEL_UNREACHABLE,result.status);
      TEST_ASSERT_FLOAT_WITHIN(0.05f,target,estimate.totalLpm);
    }
  }
}
void test_model_status_from_actual_power() {
  TEST_ASSERT_EQUAL(FLOW_MODEL_NONE,flowModelStatusForPower(40.0f,false));
  TEST_ASSERT_EQUAL(FLOW_MODEL_NONE,flowModelStatusForPower(0.0f,true));
  TEST_ASSERT_EQUAL(FLOW_MODEL_BELOW_CALIBRATED_RANGE,flowModelStatusForPower(4.9f,true));
  TEST_ASSERT_EQUAL(FLOW_MODEL_OK,flowModelStatusForPower(5.0f,true));
  TEST_ASSERT_EQUAL(FLOW_MODEL_EXTRAPOLATED,flowModelStatusForPower(60.1f,true));
}
void test_flow_setpoint_none_scale_and_pending_apply() {
  FlowSetpointState state;
  flowSetpointInitialize(state,false,0.0f);
  TEST_ASSERT_FALSE(state.appliedDefined); TEST_ASSERT_FALSE(flowSetpointAdjust(state,-1,FLOW_SETPOINT_MIN_LPM,FLOW_SETPOINT_MAX_LPM,FLOW_SETPOINT_EDIT_STEP_LPM,0));
  TEST_ASSERT_TRUE(flowSetpointAdjust(state,1,FLOW_SETPOINT_MIN_LPM,FLOW_SETPOINT_MAX_LPM,FLOW_SETPOINT_EDIT_STEP_LPM,1));
  TEST_ASSERT_TRUE(state.editing); TEST_ASSERT_TRUE(state.pendingDefined); TEST_ASSERT_FLOAT_WITHIN(0.001f,30.0f,state.pendingLpm);
  TEST_ASSERT_TRUE(flowSetpointAdjust(state,-1,FLOW_SETPOINT_MIN_LPM,FLOW_SETPOINT_MAX_LPM,FLOW_SETPOINT_EDIT_STEP_LPM,2));
  TEST_ASSERT_FALSE(state.pendingDefined); TEST_ASSERT_TRUE(flowSetpointCommit(state)); TEST_ASSERT_FALSE(state.appliedDefined);
  flowSetpointInitialize(state,true,245.0f);
  TEST_ASSERT_TRUE(flowSetpointAdjust(state,1,FLOW_SETPOINT_MIN_LPM,FLOW_SETPOINT_MAX_LPM,FLOW_SETPOINT_EDIT_STEP_LPM,3));
  TEST_ASSERT_FLOAT_WITHIN(0.001f,250.0f,state.pendingLpm);
  TEST_ASSERT_TRUE(flowSetpointAdjust(state,1,FLOW_SETPOINT_MIN_LPM,FLOW_SETPOINT_MAX_LPM,FLOW_SETPOINT_EDIT_STEP_LPM,4));
  TEST_ASSERT_FLOAT_WITHIN(0.001f,255.0f,state.pendingLpm);
  flowSetpointInitialize(state,true,495.0f);
  TEST_ASSERT_TRUE(flowSetpointAdjust(state,1,FLOW_SETPOINT_MIN_LPM,FLOW_SETPOINT_MAX_LPM,FLOW_SETPOINT_EDIT_STEP_LPM,5));
  TEST_ASSERT_FLOAT_WITHIN(0.001f,500.0f,state.pendingLpm);
  TEST_ASSERT_FALSE(flowSetpointAdjust(state,1,FLOW_SETPOINT_MIN_LPM,FLOW_SETPOINT_MAX_LPM,FLOW_SETPOINT_EDIT_STEP_LPM,6));
  TEST_ASSERT_FLOAT_WITHIN(0.001f,500.0f,state.pendingLpm);
  TEST_ASSERT_TRUE(flowSetpointAdjust(state,-1,FLOW_SETPOINT_MIN_LPM,FLOW_SETPOINT_MAX_LPM,FLOW_SETPOINT_EDIT_STEP_LPM,7));
  TEST_ASSERT_FLOAT_WITHIN(0.001f,495.0f,state.pendingLpm);
  flowSetpointInitialize(state,false,0.0f);
  for(uint16_t i=0;i<95;++i) flowSetpointAdjust(state,1,FLOW_SETPOINT_MIN_LPM,FLOW_SETPOINT_MAX_LPM,FLOW_SETPOINT_EDIT_STEP_LPM,10+i);
  TEST_ASSERT_TRUE(state.pendingDefined); TEST_ASSERT_FLOAT_WITHIN(0.001f,500.0f,state.pendingLpm);
  TEST_ASSERT_TRUE(flowSetpointCommit(state)); TEST_ASSERT_TRUE(state.appliedDefined); TEST_ASSERT_FLOAT_WITHIN(0.001f,500.0f,state.appliedLpm);
  for(uint16_t i=0;i<94;++i) flowSetpointAdjust(state,-1,FLOW_SETPOINT_MIN_LPM,FLOW_SETPOINT_MAX_LPM,FLOW_SETPOINT_EDIT_STEP_LPM,200+i);
  TEST_ASSERT_FLOAT_WITHIN(0.001f,30.0f,state.pendingLpm);
  TEST_ASSERT_TRUE(flowSetpointAdjust(state,-1,FLOW_SETPOINT_MIN_LPM,FLOW_SETPOINT_MAX_LPM,FLOW_SETPOINT_EDIT_STEP_LPM,300)); TEST_ASSERT_FALSE(state.pendingDefined);
  TEST_ASSERT_TRUE(flowSetpointCommit(state)); TEST_ASSERT_FALSE(state.appliedDefined);
  TEST_ASSERT_TRUE(flowSetpointValueAllowed(30.0f,FLOW_SETPOINT_MIN_LPM,FLOW_SETPOINT_MAX_LPM,FLOW_SETPOINT_EDIT_STEP_LPM));
  TEST_ASSERT_TRUE(flowSetpointValueAllowed(500.0f,FLOW_SETPOINT_MIN_LPM,FLOW_SETPOINT_MAX_LPM,FLOW_SETPOINT_EDIT_STEP_LPM));
  TEST_ASSERT_FALSE(flowSetpointValueAllowed(0.0f,FLOW_SETPOINT_MIN_LPM,FLOW_SETPOINT_MAX_LPM,FLOW_SETPOINT_EDIT_STEP_LPM));
  TEST_ASSERT_FALSE(flowSetpointValueAllowed(505.0f,FLOW_SETPOINT_MIN_LPM,FLOW_SETPOINT_MAX_LPM,FLOW_SETPOINT_EDIT_STEP_LPM));
  TEST_ASSERT_FALSE(flowSetpointValueAllowed(32.0f,FLOW_SETPOINT_MIN_LPM,FLOW_SETPOINT_MAX_LPM,FLOW_SETPOINT_EDIT_STEP_LPM));
}
void test_flow_setpoint_timeout_and_wraparound() {
  FlowSetpointState state;
  flowSetpointInitialize(state,true,495.0f);
  flowSetpointAdjust(state,1,FLOW_SETPOINT_MIN_LPM,FLOW_SETPOINT_MAX_LPM,FLOW_SETPOINT_EDIT_STEP_LPM,0xFFFFFFF0UL);
  TEST_ASSERT_FLOAT_WITHIN(0.001f,500.0f,state.pendingLpm);
  TEST_ASSERT_FALSE(flowSetpointTimedOut(state,0x00001377UL,5000));
  TEST_ASSERT_TRUE(flowSetpointTimedOut(state,0x00001378UL,5000));
  TEST_ASSERT_FALSE(state.editing); TEST_ASSERT_TRUE(state.pendingDefined); TEST_ASSERT_FLOAT_WITHIN(0.001f,495.0f,state.pendingLpm);
}
void test_model_estimates_are_not_clamped_by_user_target_limit() {
  const FlowEstimate estimate=estimateFlow(100.0f,0);
  TEST_ASSERT_TRUE(estimate.output1Lpm>250.0f);
  TEST_ASSERT_TRUE(estimate.output2Lpm>250.0f);
  TEST_ASSERT_TRUE(estimate.totalLpm>500.0f);
}
void test_temperature_fault_state() {
  TemperatureFaultState sensor;
  temperatureStateInitialize(sensor);
  TEST_ASSERT_FALSE(temperatureStateUsable(sensor,0,2500));
  temperatureStateRecordSuccess(sensor,24.5f,100);
  TEST_ASSERT_TRUE(temperatureStateUsable(sensor,200,2500));
  temperatureStateRecordFailure(sensor,200,3,2500);
  TEST_ASSERT_TRUE(sensor.valid); TEST_ASSERT_FALSE(sensor.stale); TEST_ASSERT_EQUAL_UINT8(1,sensor.failCount);
  temperatureStateRecordFailure(sensor,300,3,2500); temperatureStateRecordFailure(sensor,400,3,2500);
  TEST_ASSERT_FALSE(sensor.valid); TEST_ASSERT_TRUE(sensor.stale);
  temperatureStateRecordSuccess(sensor,26.0f,0xFFFFFF00UL);
  temperatureStateRefresh(sensor,0x00000900UL,2500); TEST_ASSERT_TRUE(sensor.stale);
}
void test_timer_and_format() {
  char b[16]; formatElapsed(61000,b,sizeof(b)); TEST_ASSERT_EQUAL_STRING("01:01",b);
  formatElapsed(3661000,b,sizeof(b)); TEST_ASSERT_EQUAL_STRING("1:01:01",b);
  char d[24]; formatFixed1(23.14f,d,sizeof(d)); TEST_ASSERT_EQUAL_STRING("23,1",d);
  formatFixed1(500.0f,d,sizeof(d)); TEST_ASSERT_EQUAL_STRING("500,0",d);
}
void test_manual_fan_apply_and_transition() {
  PendingApplyState manual;
  pendingApplyInitialize(manual,40.0f); pendingApplyAdjust(manual,1,1.0f,0.0f,100.0f,0);
  TEST_ASSERT_TRUE(pendingApplyCommit(manual));
  TEST_ASSERT_EQUAL_UINT8(148,fanCommandToPwm(42.0f,true));
  TEST_ASSERT_EQUAL_UINT8(107,fanCommandToPwm(42.0f,false));
  TEST_ASSERT_FLOAT_WITHIN(0.001f,70.0f,activeFanCommandPercent(true,manual.applied,70.0f));
  TEST_ASSERT_FLOAT_WITHIN(0.001f,70.0f,manualPowerAfterAutoFallback(70.0f));
}
void test_pending_gate_and_timeout() {
  PendingApplyState gate;
  pendingApplyInitialize(gate,0.0f); pendingApplyAdjust(gate,-1,10.0f,-100.0f,100.0f,0);
  TEST_ASSERT_FLOAT_WITHIN(0.001f,0.0f,gate.applied); TEST_ASSERT_TRUE(pendingApplyCommit(gate));
  TEST_ASSERT_FLOAT_WITHIN(0.001f,-10.0f,gate.applied); TEST_ASSERT_FLOAT_WITHIN(0.001f,45.0f,signedGateToPhysicalPercent(gate.applied));
  pendingApplyAdjust(gate,1,10.0f,-100.0f,100.0f,100); TEST_ASSERT_TRUE(pendingApplyTimedOut(gate,5100,5000));
  TEST_ASSERT_FLOAT_WITHIN(0.001f,-10.0f,gate.pending);
}
int main(int,char**) {
  UNITY_BEGIN();
  RUN_TEST(test_model_direct_reference_curve); RUN_TEST(test_model_zero_and_nonnegative);
  RUN_TEST(test_negative_gate_interpolation); RUN_TEST(test_positive_gate_derived_geometry);
  RUN_TEST(test_model_inverse_and_status); RUN_TEST(test_model_inverse_for_500_lpm_and_unreachable_status); RUN_TEST(test_model_status_from_actual_power);
  RUN_TEST(test_flow_setpoint_none_scale_and_pending_apply); RUN_TEST(test_flow_setpoint_timeout_and_wraparound);
  RUN_TEST(test_temperature_fault_state); RUN_TEST(test_timer_and_format);
  RUN_TEST(test_manual_fan_apply_and_transition); RUN_TEST(test_pending_gate_and_timeout); RUN_TEST(test_model_estimates_are_not_clamped_by_user_target_limit);
  return UNITY_END();
}
