#ifndef RGBLED_H
#define RGBLED_H

#include "mbed.h"

class RGBLed {
public:
    RGBLed(PinName red_pin, PinName green_pin, PinName blue_pin);

    void srgb(float r, float g, float b);
    void hsv(float H, float S, float V);

private:
    PwmOut _red;
    PwmOut _green;
    PwmOut _blue;

    float _fclamp(float a, float min, float max);
};

#endif // RGBLED_H