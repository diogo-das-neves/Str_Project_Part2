#include "RGBled.h"

#include "mbed.h"
#include "math.h"

//Constructor
RGBLed::RGBLed(PinName red_pin, PinName green_pin, PinName blue_pin) 
    : _red(red_pin), _green(green_pin), _blue(blue_pin) 
{
    _red = 1.0f;
    _green = 1.0f;
    _blue = 1.0f;

    // Set PWM frequency to 20kHz
    _red.period(0.00005f);
    _green.period(0.00005f);
    _blue.period(0.00005f);
}

float RGBLed::_fclamp(float a, float min, float max) {
    if (a > max) return max;
    if (a < min) return min;
    return a;
}

/*-------------------------------------------------------------------------+
| applies gamma corrected SRGB value to LED, inputs 0.0 to 1.0
| using basic linear color correction values from https://github.com/FastLED/FastLED/tree/master/src
+--------------------------------------------------------------------------*/ 
void RGBLed::srgb(float r, float g, float b) {
    _red = _fclamp(1.0f - 1.00f * (r > 0.04045f ? pow((r + 0.055f)/1.055f, 2.4f) : r/12.92f), 0, 1);
    _green = _fclamp(1.0f - 0.69f * (g > 0.04045f ? pow((g + 0.055f)/1.055f, 2.4f) : g/12.92f), 0, 1);
    _blue = _fclamp(1.0f - 0.94f * (b > 0.04045f ? pow((b + 0.055f)/1.055f, 2.4f) : b/12.92f), 0, 1);
}

/*-------------------------------------------------------------------------+
| applies gamma corrected HSV color value to LED
| H: hue, 0.0 to 360.0 degrees:
    0 = red, 60 = yellow, 120 = green, 180 = cyan, 240 = blue, 300 = magenta, 360 = red
| S: saturation, 0.0 to 1.0
| V: value ("brightness"), 0.0 to 1.0
+--------------------------------------------------------------------------*/ 
void RGBLed::hsv(float H, float S, float V) {
    float r, g, b;
    float max = V;
    float min = V * (1.0f - S);
    float slope = (max - min) / 60.0f;

    
    if ((H >= 0.0f) && (H < 60.0f)) {
        r = max; g = min + slope * (H - 0.0f); b = min;
    } else if ((H >= 60.0f) && (H < 120.0f)) {
        r = max - slope * (H - 60.0f); g = max; b = min;
    } else if ((H >= 120.0f) && (H < 180.0f)) {
        r = min; g = max; b = min + slope * (H - 120.0f);
    } else if ((H >= 180.0f) && (H < 240.0f)) {
        r = min; g = max - slope * (H - 180.0f); b = max;
    } else if ((H >= 240.0f) && (H < 300.0f)) {
        r = min + slope * (H - 240.0f); g = min; b = max;
    } else {
        r = max; g = min; b = max - slope * (H - 300.0f);
    }

    srgb(r, g, b);
}