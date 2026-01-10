#ifndef KILLBIT_H
#define KILLBIT_H

#include "mbed.h"

class KillBit {
public:
    KillBit(PinName buttonPin, PinName l1, PinName l2, PinName l3, PinName l4);
    void update();
    void reset();
    void off();
    
private:
    void _blinkWin();
    void _display();

    DigitalIn _btn;
    DigitalOut _led1, _led2, _led3, _led4;
    unsigned int _pattern;
};

#endif
