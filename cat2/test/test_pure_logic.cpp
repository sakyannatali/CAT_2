#include <unity.h>
#include <cstring>
#include "pure_logic.h"
#include "generated/air_density_table.h"

void setUp() {} void tearDown() {}

void test_frequency() {
  TEST_ASSERT_TRUE(frequencyFromCount(200,1000000)>=200.0f);
  TEST_ASSERT_TRUE(hybridFrequency(3,1000000,0,100,3000)>hybridFrequency(2,1000000,0,100,3000));
}
void test_low_frequency_and_zero_timeout() {
  TEST_ASSERT_FLOAT_WITHIN(0.01f,2.0f,hybridFrequency(0,1000000,500000,1500,3000));
  TEST_ASSERT_EQUAL_FLOAT(0.0f,hybridFrequency(0,1000000,500000,3001,3000));
}
void test_experimental_flow_formulas() {
  TEST_ASSERT_FLOAT_WITHIN(0.0001f,56.8193f,baseFlow1Lpm(31.9f));
  TEST_ASSERT_FLOAT_WITHIN(0.0001f,5.348758f,baseFlow2Lpm(0.0f));
  TEST_ASSERT_EQUAL_FLOAT(0.0f,correctedFlowLpm(1,0.0f,1.0f,true));
  TEST_ASSERT_EQUAL_FLOAT(0.0f,baseFlow1Lpm(0.0f));
}
void test_density_table() {
  float previous=0.0f;
  for(uint8_t i=0;i<AIR_DENSITY_TABLE_COUNT;++i) {
    const float value=airDensityTableValue(i);
    TEST_ASSERT_TRUE(value>0.0f);
    if(i) TEST_ASSERT_TRUE(value>=previous);
    previous=value;
  }
  float correction=0.0f;
  TEST_ASSERT_TRUE(airDensityCorrectionAt(22.9f,correction));
  TEST_ASSERT_FLOAT_WITHIN(0.00002f,1.00114379f,correction);
  TEST_ASSERT_FALSE(airDensityCorrectionAt(-20.1f,correction));
  TEST_ASSERT_FALSE(airDensityCorrectionAt(80.1f,correction));
}
void test_five_second_moving_average() {
  TimedMovingAverage<12> average;
  uint8_t n=0;
  for(uint32_t t=0;t<=5000;t+=500) average.add(10.0f,t);
  TEST_ASSERT_FLOAT_WITHIN(0.001f,10.0f,average.value(5000,5000,&n));
  TEST_ASSERT_EQUAL_UINT8(11,n);
  average.add(20.0f,5500); // step response: previous 10 samples plus one 20 sample.
  TEST_ASSERT_FLOAT_WITHIN(0.01f,10.909f,average.value(5500,5000,&n));
  // Invalid samples are never added. Confirmed zero is explicitly a valid sample.
  average.clear(); average.add(20.0f,0); average.add(0.0f,500);
  TEST_ASSERT_FLOAT_WITHIN(0.001f,10.0f,average.value(500,5000,&n));
}
void test_display_interval_and_feedback_selection() {
  TEST_ASSERT_FALSE(intervalElapsed(4999,0,5000));
  TEST_ASSERT_TRUE(intervalElapsed(5000,0,5000));
  TEST_ASSERT_TRUE(intervalElapsed(0x00000010UL,0xFFFFFFF0UL,32));
  float feedback;
  TEST_ASSERT_TRUE(averageOfValid(true,12.0f,false,0.0f,feedback));
  TEST_ASSERT_FLOAT_WITHIN(0.001f,12.0f,feedback);
  TEST_ASSERT_TRUE(averageOfValid(true,12.0f,true,18.0f,feedback));
  TEST_ASSERT_FLOAT_WITHIN(0.001f,15.0f,feedback);
  TEST_ASSERT_FALSE(averageOfValid(false,0.0f,false,0.0f,feedback));
}
void test_pi() {
  PiControllerCore pi;
  pi.kp=10; pi.tiSeconds=1; pi.reset(95);
  for(int i=0;i<100;++i) pi.update(10,1,100,2);
  TEST_ASSERT_TRUE(pi.output<=100.0f);
  TEST_ASSERT_TRUE(pi.integral<=2.001f);
  pi.kp=0.5f; pi.tiSeconds=100.0f;
  pi.makeBumpless(0.2f,42.0f);
  TEST_ASSERT_FLOAT_WITHIN(0.01f,42.0f,pi.output);
  const PiTerms after=pi.update(0.2f,1.0f,5.0f,20000.0f);
  TEST_ASSERT_FLOAT_WITHIN(0.1f,42.0f,after.requested);
}
void test_timer_and_format() {
  char b[16]; formatElapsed(61000,b,sizeof(b)); TEST_ASSERT_EQUAL_STRING("01:01",b);
  formatElapsed(3661000,b,sizeof(b)); TEST_ASSERT_EQUAL_STRING("1:01:01",b);
  char d[24]; formatFixed1(23.14f,d,sizeof(d)); TEST_ASSERT_EQUAL_STRING("23,1",d);
  char labelled[24]; snprintf(labelled,sizeof(labelled),"%s L/min",d);
  TEST_ASSERT_TRUE(strstr(labelled,"23,1")!=0);
}
void test_pending_adjust_and_bounds() {
  PendingApplyState vent;
  pendingApplyInitialize(vent,40.0f);
  pendingApplyAdjust(vent,1,1.0f,0.0f,100.0f,100);
  TEST_ASSERT_FLOAT_WITHIN(0.001f,40.0f,vent.applied);
  TEST_ASSERT_FLOAT_WITHIN(0.001f,41.0f,vent.pending);
  for(uint8_t i=0;i<4;++i) pendingApplyAdjust(vent,1,1.0f,0.0f,100.0f,101+i);
  TEST_ASSERT_FLOAT_WITHIN(0.001f,45.0f,vent.pending);
  pendingApplyInitialize(vent,100.0f);
  pendingApplyAdjust(vent,1,1.0f,0.0f,100.0f,200);
  TEST_ASSERT_EQUAL_FLOAT(100.0f,vent.pending);
  pendingApplyInitialize(vent,0.0f);
  pendingApplyAdjust(vent,-1,1.0f,0.0f,100.0f,200);
  TEST_ASSERT_EQUAL_FLOAT(0.0f,vent.pending);
}
void test_pending_timeout_uses_last_press() {
  PendingApplyState state;
  pendingApplyInitialize(state,40.0f);
  pendingApplyAdjust(state,1,1.0f,0.0f,100.0f,0);
  pendingApplyAdjust(state,1,1.0f,0.0f,100.0f,4000);
  TEST_ASSERT_FALSE(pendingApplyTimedOut(state,8999,5000));
  TEST_ASSERT_TRUE(state.editing);
  TEST_ASSERT_TRUE(pendingApplyTimedOut(state,9000,5000));
  TEST_ASSERT_FALSE(state.editing);
  TEST_ASSERT_FLOAT_WITHIN(0.001f,40.0f,state.pending);
  TEST_ASSERT_FLOAT_WITHIN(0.001f,40.0f,state.applied);
}
void test_pending_apply_and_expired_apply() {
  PendingApplyState state;
  pendingApplyInitialize(state,40.0f);
  pendingApplyAdjust(state,1,1.0f,0.0f,100.0f,0);
  pendingApplyAdjust(state,1,1.0f,0.0f,100.0f,1);
  TEST_ASSERT_TRUE(pendingApplyCommit(state));
  TEST_ASSERT_FLOAT_WITHIN(0.001f,42.0f,state.applied);
  TEST_ASSERT_FALSE(state.editing);
  TEST_ASSERT_FALSE(pendingApplyTimedOut(state,10000,5000));
  TEST_ASSERT_FLOAT_WITHIN(0.001f,42.0f,state.applied);
  pendingApplyAdjust(state,1,1.0f,0.0f,100.0f,11000);
  TEST_ASSERT_TRUE(pendingApplyTimedOut(state,16000,5000));
  TEST_ASSERT_FALSE(pendingApplyCommit(state));
  TEST_ASSERT_FLOAT_WITHIN(0.001f,42.0f,state.applied);
}
void test_pending_parameters_are_independent() {
  PendingApplyState vent,gate,flow;
  pendingApplyInitialize(vent,20.0f);
  pendingApplyInitialize(gate,30.0f);
  pendingApplyInitialize(flow,15.0f);
  pendingApplyAdjust(vent,1,1.0f,0.0f,100.0f,0);
  pendingApplyAdjust(gate,-1,1.0f,0.0f,100.0f,1000);
  pendingApplyAdjust(flow,1,1.0f,0.0f,100.0f,2000);
  TEST_ASSERT_TRUE(pendingApplyTimedOut(vent,5000,5000));
  TEST_ASSERT_FALSE(pendingApplyTimedOut(gate,5000,5000));
  TEST_ASSERT_FALSE(pendingApplyTimedOut(flow,5000,5000));
  TEST_ASSERT_FLOAT_WITHIN(0.001f,15.0f,flow.applied); // PI must keep this until Apply.
  TEST_ASSERT_FLOAT_WITHIN(0.001f,16.0f,flow.pending);
  TEST_ASSERT_TRUE(pendingApplyCommit(flow));
  TEST_ASSERT_FLOAT_WITHIN(0.001f,16.0f,flow.applied);
}
void test_pending_timeout_wraparound() {
  PendingApplyState state;
  pendingApplyInitialize(state,10.0f);
  pendingApplyAdjust(state,1,1.0f,0.0f,100.0f,0xFFFFFFF0UL);
  TEST_ASSERT_FALSE(pendingApplyTimedOut(state,0x00001377UL,5000));
  TEST_ASSERT_TRUE(pendingApplyTimedOut(state,0x00001378UL,5000));
}
int main(int,char**) {
  UNITY_BEGIN();
  RUN_TEST(test_frequency); RUN_TEST(test_low_frequency_and_zero_timeout);
  RUN_TEST(test_experimental_flow_formulas); RUN_TEST(test_density_table);
  RUN_TEST(test_five_second_moving_average); RUN_TEST(test_display_interval_and_feedback_selection);
  RUN_TEST(test_pi); RUN_TEST(test_timer_and_format);
  RUN_TEST(test_pending_adjust_and_bounds); RUN_TEST(test_pending_timeout_uses_last_press);
  RUN_TEST(test_pending_apply_and_expired_apply); RUN_TEST(test_pending_parameters_are_independent);
  RUN_TEST(test_pending_timeout_wraparound);
  return UNITY_END();
}
