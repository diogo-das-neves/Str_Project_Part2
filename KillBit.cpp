#include "KillBit.h"

KillBit::KillBit(PinName bPin, PinName l1, PinName l2, PinName l3, PinName l4) 
: _btn(bPin), _led1(l1), _led2(l2), _led3(l3), _led4(l4){
        reset();
}

void KillBit::off(){
    _led1= _led2 = _led3 = _led4 = 0;
}

void KillBit::reset() {
    _pattern = 0x08;
}

void KillBit::update() {
   
    if (_btn == 1) { 
        if (_pattern & 0x01) _pattern &= ~0x01; // Kill bit at LED4
        else _pattern |= 0x01;                 // Add bit at LED4
        

        while(_btn == 1) { 
            wait_ms(10); 
        }
    }

    if (_pattern == 0) {
        _blinkWin();
        reset();
    }

    // rotating bit
    unsigned int rotating_bit = (_pattern & 0x01);
    _pattern = (rotating_bit << 3) | (_pattern >> 1);
    
    _display();
}

void KillBit::_display() {
    _led4 = (_pattern) & 0x01;
    _led3 = (_pattern >> 1) & 0x01;
    _led2 = (_pattern >> 2) & 0x01;
    _led1 = (_pattern >> 3) & 0x01;
}

void KillBit::_blinkWin() {
    for(int i=0; i<3; i++) {
        _led1 = _led2 = _led3 = _led4 = 1; wait(0.2);
        _led1 = _led2 = _led3 = _led4 = 0; wait(0.2);
    }
}
