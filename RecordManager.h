#ifndef RECORDS_H
#define RECORDS_H

#include "mbed.h"
#include "extras.h" 
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "extras.h"
#include "timers.h"


class RecordManager {
public:
    RecordManager();

    void update(float currentTemp);

    void clear();
    Record getMin();
    Record getMax();


private:
    Record _min;
    Record _max;
    SemaphoreHandle_t _mutex;
};

#endif