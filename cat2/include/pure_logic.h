#pragma once

#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include <stdio.h>

inline float clampNonNegative(float value) { return value < 0.0f ? 0.0f : value; }
inline float baseFlow1Lpm(float frequencyHz) {
  return clampNonNegative(-3.334481f + 1.885698f * frequencyHz);
}
inline float baseFlow2Lpm(float frequencyHz) {
  return clampNonNegative(5.348758f + 1.453538f * frequencyHz);
}
inline float baseFlowLpm(uint8_t meterIndex, float frequencyHz) {
  return meterIndex == 0 ? baseFlow1Lpm(frequencyHz) : baseFlow2Lpm(frequencyHz);
}
inline float correctedFlowLpm(uint8_t meterIndex, float frequencyHz,
                              float densityCorrection, bool confirmedZeroFlow) {
  // Meter 2 has a positive intercept. A confirmed zero must win over its fit.
  if (confirmedZeroFlow) return 0.0f;
  return baseFlowLpm(meterIndex, frequencyHz) * densityCorrection;
}

inline float frequencyFromCount(uint32_t pulses, uint32_t windowUs) {
  return windowUs == 0 ? 0.0f : (1000000.0f * pulses) / windowUs;
}
inline float frequencyFromPeriod(uint32_t periodUs) {
  return periodUs == 0 ? 0.0f : 1000000.0f / periodUs;
}
inline float hybridFrequency(uint32_t deltaPulses, uint32_t windowUs, uint32_t lastPeriodUs,
                             uint32_t lastPulseAgeMs, uint32_t zeroTimeoutMs) {
  if (lastPulseAgeMs > zeroTimeoutMs) return 0.0f;
  if (deltaPulses >= 2 && windowUs > 0) return frequencyFromCount(deltaPulses, windowUs);
  return frequencyFromPeriod(lastPeriodUs);
}

inline bool intervalElapsed(uint32_t now, uint32_t previous, uint32_t intervalMs) {
  return (uint32_t)(now-previous) >= intervalMs;
}
inline bool averageOfValid(bool firstValid, float first, bool secondValid, float second, float &result) {
  if (!firstValid && !secondValid) { result=NAN; return false; }
  if (firstValid && secondValid) result=(first+second)*0.5f;
  else result=firstValid ? first : second;
  return true;
}

template <uint8_t Capacity>
class TimedMovingAverage {
 public:
  TimedMovingAverage() : count_(0), next_(0) { clear(); }
  void clear() { count_=0; next_=0; for (uint8_t i=0;i<Capacity;++i) valid_[i]=false; }
  void add(float value, uint32_t now) { values_[next_]=value; times_[next_]=now; valid_[next_]=true; next_=(uint8_t)((next_+1)%Capacity); if(count_<Capacity) ++count_; }
  float value(uint32_t now, uint32_t windowMs, uint8_t *validCount = 0) {
    float sum=0; uint8_t n=0;
    for(uint8_t i=0;i<count_;++i) if(valid_[i] && (uint32_t)(now-times_[i]) <= windowMs) { sum+=values_[i]; ++n; }
    if(validCount) *validCount=n;
    return n ? sum/n : NAN;
  }
 private:
  float values_[Capacity]; uint32_t times_[Capacity]; bool valid_[Capacity]; uint8_t count_, next_;
};

struct PiTerms { float error; float p; float i; float requested; };
class PiControllerCore {
 public:
  PiControllerCore() : kp(0.5f), tiSeconds(100.0f), integral(0), output(0) {}
  float kp, tiSeconds, integral, output;
  void reset(float initialOutput) { output=clamp(initialOutput); integral=0; }
  void makeBumpless(float normalizedError, float desiredOutput) {
    output=clamp(desiredOutput);
    if(kp > 0.00001f && tiSeconds > 0.00001f) integral=(output/kp-normalizedError)*tiSeconds;
  }
  PiTerms update(float normalizedError, float dtSeconds, float maxStep, float integralLimit) {
    PiTerms r={normalizedError, kp*normalizedError, kp*(integral/tiSeconds), output};
    if(!(dtSeconds > 0.0f) || tiSeconds <= 0.0f || kp < 0.0f) return r;
    const float candidateIntegral=clampRange(integral + normalizedError*dtSeconds, -integralLimit, integralLimit);
    const float unslewed=kp*(normalizedError+candidateIntegral/tiSeconds);
    const bool high=unslewed>100.0f && normalizedError>0.0f;
    const bool low=unslewed<0.0f && normalizedError<0.0f;
    if(!high && !low) integral=candidateIntegral;
    r.p=kp*normalizedError; r.i=kp*(integral/tiSeconds);
    const float target=clamp(r.p+r.i);
    output=clampRange(target, output-maxStep, output+maxStep);
    r.requested=output;
    return r;
  }
  static float clamp(float x) { return clampRange(x,0.0f,100.0f); }
  static float clampRange(float x,float lo,float hi) { return x<lo?lo:(x>hi?hi:x); }
};

inline void formatElapsed(uint32_t elapsedMs, char *out, size_t size) {
  const uint32_t total=elapsedMs/1000UL, h=total/3600UL, m=(total/60UL)%60UL, s=total%60UL;
  if(h) snprintf(out,size,"%lu:%02lu:%02lu",(unsigned long)h,(unsigned long)m,(unsigned long)s);
  else snprintf(out,size,"%02lu:%02lu",(unsigned long)m,(unsigned long)s);
}
inline void formatFixed1(float value, char *out, size_t size) {
  const long scaled=(long)(value >= 0 ? value*10.0f+0.5f : value*10.0f-0.5f);
  const long absScaled=scaled<0?-scaled:scaled;
  snprintf(out,size,"%s%ld,%ld",scaled<0?"-":"",absScaled/10L,absScaled%10L);
}
