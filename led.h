#ifndef LED_H
#define LED_H

#include "RTC.h"

void initLED(void);
void srgbLED(float r,float g,float b);
void hsvLED(float H, float S, float V);

#endif // LED_H