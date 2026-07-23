#include <unity.h>
#include "pure_logic.h"
void setUp(){} void tearDown(){}
void test_frequency(){TEST_ASSERT_TRUE(frequencyFromCount(200,1000000)>=200.0f);TEST_ASSERT_TRUE(hybridFrequency(3,1000000,0,100,3000)>hybridFrequency(2,1000000,0,100,3000));}
void test_low_frequency(){TEST_ASSERT_FLOAT_WITHIN(0.01f,2.0f,hybridFrequency(0,1000000,500000,1500,3000));TEST_ASSERT_EQUAL_FLOAT(0.0f,hybridFrequency(0,1000000,500000,3001,3000));}
void test_calibration(){CalibrationPoint p[]={{0,0},{10,100},{20,300}};TEST_ASSERT_TRUE(calibrationTableValid(p,3));TEST_ASSERT_FLOAT_WITHIN(0.01f,200,interpolateCalibration(p,3,15));CalibrationPoint bad[]={{0,0},{0,1}};TEST_ASSERT_FALSE(calibrationTableValid(bad,2));}
void test_pi(){PiControllerCore pi;pi.kp=10;pi.tiSeconds=1;pi.reset(95);for(int i=0;i<100;++i)pi.update(10,1,100,2);TEST_ASSERT_TRUE(pi.output<=100);TEST_ASSERT_TRUE(pi.integral<=2.001f);pi.makeBumpless(0,42);TEST_ASSERT_FLOAT_WITHIN(0.01f,42,pi.output);}
void test_timer_and_format(){char b[16];formatElapsed(61000,b,sizeof(b));TEST_ASSERT_EQUAL_STRING("01:01",b);formatElapsed(3661000,b,sizeof(b));TEST_ASSERT_EQUAL_STRING("1:01:01",b);TEST_ASSERT_EQUAL_UINT32(32,(uint32_t)(0x00000010UL-0xFFFFFFF0UL));char d[16];formatFixed1(23.14f,d,sizeof(d));TEST_ASSERT_EQUAL_STRING("23,1",d);}
int main(int,char**){UNITY_BEGIN();RUN_TEST(test_frequency);RUN_TEST(test_low_frequency);RUN_TEST(test_calibration);RUN_TEST(test_pi);RUN_TEST(test_timer_and_format);return UNITY_END();}
