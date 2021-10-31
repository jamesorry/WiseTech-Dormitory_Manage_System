#ifndef _TIMER_H_
#define _TIMER_H_

#define TIMER_INTERVAL_MS  10

void TimerInit(unsigned char timer, unsigned long microSec);
void setTimeoutBegin(void);
unsigned int getTimeoutSec();
void setTimeoutSecond(unsigned int timeoutSecond);

#endif
