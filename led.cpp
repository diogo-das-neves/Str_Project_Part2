#include "led.h"
#include "mbed.h"
#include "math.h"

PwmOut LEDr(p23);
PwmOut LEDg(p24);
PwmOut LEDb(p25);

// helper function
float fclamp(float a, float min, float max) {
    if (a>max) return max;
    if (a<min) return min;
    return a;
}


/*-------------------------------------------------------------------------+
| set up LEDs
+--------------------------------------------------------------------------*/ 
void initLED(void) {
    LEDr = 1.0; // 1 means off
    LEDg = 1.0;
    LEDb = 1.0;

    LEDr.period(0.00005); // some people (including me) get headaches from PWM frequencies
    LEDg.period(0.00005); // under a few kHz. Setting to 20kHz. Kat
    LEDb.period(0.00005);
}

/*-------------------------------------------------------------------------+
| applies gamma corrected SRGB value to LED, inputs 0.0 to 1.0
| using basic linear color correction values from https://github.com/FastLED/FastLED/tree/master/src
+--------------------------------------------------------------------------*/ 
void srgbLED(float r,float g,float b) {
    LEDr = fclamp(1 - 1.00 * (r > 0.04045 ? pow((r + 0.055)/1.055, 2.4) : r/12.92), 0, 1) ;
    LEDg = fclamp(1 - 0.69 * (g > 0.04045 ? pow((g + 0.055)/1.055, 2.4) : g/12.92), 0, 1) ;
    LEDb = fclamp(1 - 0.94 * (b > 0.04045 ? pow((b + 0.055)/1.055, 2.4) : b/12.92), 0, 1) ;
}

/*-------------------------------------------------------------------------+
| applies gamma corrected HSV color value to LED
| H: hue, 0.0 to 360.0 degrees:
    0 = red, 60 = yellow, 120 = green, 180 = cyan, 240 = blue, 300 = magenta, 360 = red
| S: saturation, 0.0 to 1.0
| V: value ("brightness"), 0.0 to 1.0
+--------------------------------------------------------------------------*/ 
void hsvLED(float H, float S, float V) {
    float r,g,b;
    
    float max = V;
    float min = V*(1-S);
    float slope = (max - min) / 60.0;

    if((H >= 0.0)&&(H < 60.0)){
        r = max; 
        g = min + slope * (H-0.0); 
        b = min;
    }
    if((H >= 60.0)&&(H < 120.0)){
        r = max - slope * (H - 60.0); 
        g = max; 
        b = min;
    }
    if((H >= 120.0)&&(H < 180.0)){
        r = min; 
        g = max; 
        b = min + slope * (H-120.0);
    }
    if((H >= 180.0)&&(H < 240.0)){
        r = min; 
        g = max - slope * (H - 180.0); 
        b = max;
    }
    if((H >= 240.0)&&(H < 300.0)){
        r = min + slope * (H - 240.0); 
        g = min;
        b = max;
    }
    if((H >= 300.0)&&(H <= 360.0)){
        r = max; 
        g = min; 
        b = max - slope * (H - 300.0);
    }

    srgbLED(r,g,b);
}