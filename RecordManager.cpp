#include "RecordManager.h"

RecordManager::RecordManager() {
    _mutex = xSemaphoreCreateMutex();
    clear();
}

void RecordManager::clear() {
    if (xSemaphoreTake(_mutex, portMAX_DELAY)) {
        _min.temp = 99;
        _max.temp = -99;
        
        
        tm defaultTime = {0};
        defaultTime.tm_mday = 1;
        defaultTime.tm_year = 70;
        
        _min.timestamp = defaultTime;
        _max.timestamp = defaultTime;
        
        xSemaphoreGive(_mutex);
    }
}

void RecordManager::update(float currentTemp) {
    time_t t = time(NULL);
    tm now;
    localtime_r(&t, &now);

    if (xSemaphoreTake(_mutex, portMAX_DELAY)) {
        if (currentTemp > _max.temp) {
            _max.temp = (int)currentTemp;
            _max.timestamp = now;
        }
        if (currentTemp < _min.temp) {
            _min.temp = (int)currentTemp;
            _min.timestamp = now;
        }
        xSemaphoreGive(_mutex);
    }
}

Record RecordManager::getMin() {
    Record temp;
    if (xSemaphoreTake(_mutex, portMAX_DELAY)) {
        temp = _min;
        xSemaphoreGive(_mutex);
    }
    return temp;
}

Record RecordManager::getMax() {
    Record temp;
    if (xSemaphoreTake(_mutex, portMAX_DELAY)) {
        temp = _max;
        xSemaphoreGive(_mutex);
    }
    return temp;
}