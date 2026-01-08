#ifndef EXTRAS_H
#define EXTRAS_H

#include "RTC.h"


typedef struct {
    int temp;
    tm timestamp;
} Record;


/*-------------------------------------------------------------------------+
| Helper macros: avoid checking the mutex by hand every time
+--------------------------------------------------------------------------*/ 
#define MUTEX_TAKE(MUTEX) \
    if(xSemaphoreTake(MUTEX, 100)) {

#define MUTEX_RETURN(MUTEX) \
        xSemaphoreGive(MUTEX);\
    } else {\
        printf("Failed to acquire %s\n", #MUTEX);\
    }


#endif // EXTRAS_H
