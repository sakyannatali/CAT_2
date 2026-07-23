#include "timer_service.h"
#include "app_state.h"
#include "pure_logic.h"
static uint32_t startedAt=0;
void timerServiceBegin(uint32_t now){startedAt=now;}
void timerServiceTick(uint32_t now){if(app.timerRunning)app.timerElapsedMs+=(uint32_t)(now-startedAt),startedAt=now;}
void timerServiceStartOrPause(uint32_t now){if(app.timerRunning)timerServicePause(now);else timerServiceStart(now);}
void timerServiceStart(uint32_t now){if(!app.timerRunning){startedAt=now;app.timerRunning=true;}}
void timerServicePause(uint32_t now){timerServiceTick(now);app.timerRunning=false;}
void timerServiceReset(uint32_t now){app.timerRunning=false;app.timerElapsedMs=0;startedAt=now;}
void timerServiceFormat(char*out,size_t size){formatElapsed(app.timerElapsedMs,out,size);}
