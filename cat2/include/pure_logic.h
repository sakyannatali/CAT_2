#pragma once

#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include <stdio.h>

inline float clampNonNegative(float value) { return value < 0.0f ? 0.0f : value; }
inline bool intervalElapsed(uint32_t now, uint32_t previous, uint32_t intervalMs) {
  return (uint32_t)(now-previous) >= intervalMs;
}
struct PendingApplyState {
  float applied, pending;
  bool editing;
  uint32_t lastEditMs;
};
inline float clampValue(float value, float minimum, float maximum) {
  return value < minimum ? minimum : (value > maximum ? maximum : value);
}
inline float snapToStep(float value, float step, float minimum, float maximum) {
  if (!(step > 0.0f)) return clampValue(value,minimum,maximum);
  const float snapped=(float)((long)(value/step+(value>=0.0f?0.5f:-0.5f)))*step;
  return clampValue(snapped,minimum,maximum);
}
inline float signedGateToPhysicalPercent(float signedPercent) {
  return (clampValue(signedPercent,-100.0f,100.0f)+100.0f)*0.5f;
}
inline float activeFanCommandPercent(bool automaticMode, float manualApplied, float automaticOutput) {
  return automaticMode ? clampValue(automaticOutput,0.0f,100.0f) : clampValue(manualApplied,0.0f,100.0f);
}
inline bool fanSetpointUsesAutoLabel(bool autoMode) { return autoMode; }
inline float manualPowerAfterAutoFallback(float actualPower) {
  return clampValue(actualPower,0.0f,100.0f);
}
inline uint8_t fanCommandToPwm(float commandPercent, bool inverted) {
  const uint8_t direct=(uint8_t)(clampValue(commandPercent,0.0f,100.0f)*255.0f/100.0f+0.5f);
  return inverted ? (uint8_t)(255U-direct) : direct;
}
struct TemperatureFaultState {
  float value, lastGoodValue;
  uint32_t lastGoodMs;
  uint8_t failCount;
  bool hasLastGood, valid, stale;
};
inline void temperatureStateInitialize(TemperatureFaultState &state) {
  state.value=NAN; state.lastGoodValue=NAN; state.lastGoodMs=0; state.failCount=0;
  state.hasLastGood=false; state.valid=false; state.stale=false;
}
inline uint32_t temperatureStateAgeMs(const TemperatureFaultState &state, uint32_t now) {
  return state.hasLastGood ? (uint32_t)(now-state.lastGoodMs) : UINT32_MAX;
}
inline void temperatureStateRefresh(TemperatureFaultState &state, uint32_t now, uint32_t staleTimeoutMs) {
  if (state.hasLastGood && intervalElapsed(now,state.lastGoodMs,staleTimeoutMs)) {
    state.valid=false;
    state.stale=true;
  }
}
inline bool temperatureStateUsable(const TemperatureFaultState &state, uint32_t now, uint32_t staleTimeoutMs) {
  return state.valid && !state.stale && state.hasLastGood &&
         !intervalElapsed(now,state.lastGoodMs,staleTimeoutMs);
}
inline void temperatureStateRecordSuccess(TemperatureFaultState &state, float value, uint32_t now) {
  state.value=value; state.lastGoodValue=value; state.lastGoodMs=now; state.failCount=0;
  state.hasLastGood=true; state.valid=true; state.stale=false;
}
inline void temperatureStateRecordFailure(TemperatureFaultState &state, uint32_t now,
                                          uint8_t failLimit, uint32_t staleTimeoutMs) {
  if (state.failCount<UINT8_MAX) ++state.failCount;
  temperatureStateRefresh(state,now,staleTimeoutMs);
  if (!state.hasLastGood || state.failCount>=failLimit || state.stale) {
    state.valid=false;
    state.stale=true;
    return;
  }
  state.value=state.lastGoodValue;
  state.valid=true;
  state.stale=false;
}
inline void pendingApplyInitialize(PendingApplyState &state, float value) {
  state.applied=value;
  state.pending=value;
  state.editing=false;
  state.lastEditMs=0;
}
inline void pendingApplyAdjust(PendingApplyState &state, int8_t direction, float step,
                               float minimum, float maximum, uint32_t now) {
  if (!state.editing) state.pending=snapToStep(state.applied,step,minimum,maximum);
  state.pending=clampValue(state.pending+(direction < 0 ? -step : step),minimum,maximum);
  state.editing=true;
  state.lastEditMs=now;
}
inline bool pendingApplyTimedOut(PendingApplyState &state, uint32_t now, uint32_t timeoutMs) {
  if (!state.editing || !intervalElapsed(now,state.lastEditMs,timeoutMs)) return false;
  state.pending=state.applied;
  state.editing=false;
  return true;
}
inline bool pendingApplyCommit(PendingApplyState &state) {
  if (!state.editing) return false;
  state.applied=state.pending;
  state.editing=false;
  return true;
}
inline uint32_t pendingApplyAgeMs(const PendingApplyState &state, uint32_t now) {
  return state.editing ? (uint32_t)(now-state.lastEditMs) : 0;
}
inline uint32_t pendingApplyRemainingMs(const PendingApplyState &state, uint32_t now, uint32_t timeoutMs) {
  const uint32_t age=pendingApplyAgeMs(state,now);
  return state.editing && age<timeoutMs ? timeoutMs-age : 0;
}
struct FlowSetpointState {
  bool appliedDefined, pendingDefined, editing;
  float appliedLpm, pendingLpm;
  uint32_t lastEditMs;
};
inline bool flowSetpointValueAllowed(float value, float minimum, float maximum, float step) {
  if (value < minimum || value > maximum || !(step > 0.0f)) return false;
  const float steps=(value-minimum)/step;
  const long rounded=(long)(steps+0.5f);
  return fabsf(steps-(float)rounded)<0.001f;
}
inline void flowSetpointInitialize(FlowSetpointState &state, bool defined, float value) {
  state.appliedDefined=defined; state.pendingDefined=defined;
  state.appliedLpm=defined ? value : 0.0f; state.pendingLpm=state.appliedLpm;
  state.editing=false; state.lastEditMs=0;
}
inline bool flowSetpointAdjust(FlowSetpointState &state, int8_t direction, float minimum,
                               float maximum, float step, uint32_t now) {
  if (!state.editing) { state.pendingDefined=state.appliedDefined; state.pendingLpm=state.appliedLpm; }
  bool changed=false;
  if (direction > 0) {
    if (!state.pendingDefined) { state.pendingDefined=true; state.pendingLpm=minimum; changed=true; }
    else { const float next=clampValue(state.pendingLpm+step,minimum,maximum); changed=next!=state.pendingLpm; state.pendingLpm=next; }
  } else if (state.pendingDefined) {
    if (state.pendingLpm<=minimum) { state.pendingDefined=false; state.pendingLpm=0.0f; changed=true; }
    else { const float next=clampValue(state.pendingLpm-step,minimum,maximum); changed=next!=state.pendingLpm; state.pendingLpm=next; }
  }
  if (changed || state.editing) { state.editing=true; state.lastEditMs=now; }
  return changed;
}
inline bool flowSetpointCommit(FlowSetpointState &state) {
  if (!state.editing) return false;
  state.appliedDefined=state.pendingDefined; state.appliedLpm=state.pendingLpm;
  state.editing=false;
  return true;
}
inline bool flowSetpointTimedOut(FlowSetpointState &state, uint32_t now, uint32_t timeoutMs) {
  if (!state.editing || !intervalElapsed(now,state.lastEditMs,timeoutMs)) return false;
  state.pendingDefined=state.appliedDefined; state.pendingLpm=state.appliedLpm; state.editing=false;
  return true;
}
inline uint32_t flowSetpointAgeMs(const FlowSetpointState &state, uint32_t now) {
  return state.editing ? (uint32_t)(now-state.lastEditMs) : 0;
}
inline uint32_t flowSetpointRemainingMs(const FlowSetpointState &state, uint32_t now, uint32_t timeoutMs) {
  const uint32_t age=flowSetpointAgeMs(state,now);
  return state.editing && age<timeoutMs ? timeoutMs-age : 0;
}

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
