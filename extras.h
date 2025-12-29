#ifndef RECORD_H
#define RECORD_H

#include "RTC.h"


typedef struct {
    int temp;
    tm timestamp;
} Record;
#endif
