#ifndef _MP4UTILS_H_
#define _MP4UTILS_H_

void mpSleep(unsigned int ms);

unsigned long mpClkRateGet();
unsigned long mpTickGet();
unsigned long mpTickToMs(unsigned long  dwTick);

#endif

