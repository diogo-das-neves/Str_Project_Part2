#ifndef RECORD_H
#define RECORD_H

#include "RTC.h"


struct Record {
    int temp;
    tm timestamp;
};
typedef struct Record Record; 
#endif
