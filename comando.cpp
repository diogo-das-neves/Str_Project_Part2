//#ifdef notdef

/***************************************************************************
| File: comando.c  -  Concretizacao de comandos (exemplo)
|
| Autor: Carlos Almeida (IST)
| Data:  Nov 2002
***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <climits>
#include "FreeRTOS.h"
#include "queue.h"

#include "LM75B.h"
#include "C12832.h"
#include "RTC.h"
#include "MMA7660.h"

#include "mbed.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "extras.h"
#include "task.h"

#include "mbed.h"
#include "portmacro.h"
#include "projdefs.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

#include "RecordManager.h"
#include "KillBit.h"


extern TimerHandle_t SensorTimer;


extern SemaphoreHandle_t ClockMutex;
extern SemaphoreHandle_t RecordMutex;
extern SemaphoreHandle_t ParamMutex;
extern SemaphoreHandle_t StateMutex;
extern SemaphoreHandle_t xMeasureTempSemaphore;
extern SemaphoreHandle_t TempMutex;


extern volatile float temperature; 

extern TaskHandle_t xTask_KillBitGame;
extern TaskHandle_t xTask_BubbleLevel;

extern C12832 lcd;

extern RecordManager recordLogger;
extern Record maxtemp;
extern Record mintemp;
extern volatile int low_threshold_TL;
extern volatile int high_threshold_TH;
extern volatile int monitoring_period_PMON;
extern volatile int alarm_duration_TALA;
extern volatile bool alarm_clock;
extern volatile bool temp_alarm;

bool bubble_level_bl = 1;
bool hit_bit_hb = 0;
bool config_sound_cs = 0;

extern void alarmFunction(void);
tm alarm_time = RTC::getDefaultTM();

extern KillBit bitGame;


/*-------------------------------------------------------------------------+
| Helper function: validateInput - clamp input to allowed range
+--------------------------------------------------------------------------*/ 
int validateInput(int input, int min, int max, int* output) {
    if (input > max) {
        *output = max;
        return 1;
    }
    if (input < min) {
        *output = min;
        return -1;
    }
    else {
        *output = input;
        return 0; 
    }   
}

/*-------------------------------------------------------------------------+
| Function: cmd_readdatetime - print DD/MM/YYYY hh:mm:ss
+--------------------------------------------------------------------------*/
void cmd_readdatetime(int argc, char** argv){
    MUTEX_TAKE(ClockMutex)
    tm systime_struct; time_t systime_unix;
    time(&systime_unix); // get system time
    localtime_r(&systime_unix, &systime_struct); // convert to struct form
    printf("\n%02d/%02d/%02d %02d:%02d:%02d", // using this format as per specification
        systime_struct.tm_mday, systime_struct.tm_mon + 1, systime_struct.tm_year + 1900,
        systime_struct.tm_hour, systime_struct.tm_min, systime_struct.tm_sec);
    MUTEX_RETURN(ClockMutex)
}

/*-------------------------------------------------------------------------+
| Function: cmd_readclock - print hh:mm:ss
+--------------------------------------------------------------------------*/
void cmd_readclock(int argc, char **argv) {
    MUTEX_TAKE(ClockMutex)
    tm systime_struct; time_t systime_unix;
    time(&systime_unix);
    localtime_r(&systime_unix, &systime_struct);
    printf("\n%02d:%02d:%02d", 
        systime_struct.tm_hour, systime_struct.tm_min, systime_struct.tm_sec);
    MUTEX_RETURN(ClockMutex)
}

/*-------------------------------------------------------------------------+
| Function: cmd_setdate - set the system date
+--------------------------------------------------------------------------*/
void cmd_setdate(int argc,char**argv){
    MUTEX_TAKE(ClockMutex)
    tm systime_struct; time_t systime_unix;

    time(&systime_unix);
    localtime_r(&systime_unix, &systime_struct);
    
    int err = 0;
    err |= validateInput(
        atoi(argv[1]), 1, 31, &systime_struct.tm_mday);
    err |= validateInput(
        atoi(argv[2]) - 1, 0, 11, &systime_struct.tm_mon);
    err |= validateInput(
        atoi(argv[3]) - 1900, 70, 137, &systime_struct.tm_year);
    if(err) {
        printf("\nInput out of range, clamping to %02d/%02d/%02d",
               systime_struct.tm_mday, systime_struct.tm_mon + 1, systime_struct.tm_year + 1900);
    }

    systime_unix = mktime(&systime_struct);
    set_time(systime_unix);
    MUTEX_RETURN(ClockMutex)
}

/*-------------------------------------------------------------------------+
| Function: cmd_setclock - set the system time
+--------------------------------------------------------------------------*/
void cmd_setclock(int argc, char** argv){
    MUTEX_TAKE(ClockMutex)
    tm systime_struct; time_t systime_unix;

    time(&systime_unix);
    localtime_r(&systime_unix, &systime_struct);
    
    int err = 0;
    err |= validateInput(
        atoi(argv[1]), 0, 23, &systime_struct.tm_hour);
    err |= validateInput(
        atoi(argv[2]), 0, 59, &systime_struct.tm_min);
    err |= validateInput(
        atoi(argv[3]), 0, 59, &systime_struct.tm_sec);
    if(err) {
        printf("\nInput out of range, clamping to %02d:%02d:%02d",
               systime_struct.tm_hour, systime_struct.tm_min, systime_struct.tm_sec);
    }

    systime_unix = mktime(&systime_struct);
    set_time(systime_unix);
    MUTEX_RETURN(ClockMutex)
}

/*-------------------------------------------------------------------------+
| Function: cmd_readtemp - read temperature (ondemand measurement)
+--------------------------------------------------------------------------*/
void cmd_readtemp(int argc, char **argv) {
    float sensor_read;

    xSemaphoreGive(xMeasureTempSemaphore); // request on demand measurement
    taskYIELD();
    
    MUTEX_TAKE(TempMutex)
    sensor_read = temperature; // cache temperature
    MUTEX_RETURN(TempMutex)

    printf("\nTemperature:% 2.1f", sensor_read);   
}

/*-------------------------------------------------------------------------+
| Function: cmd_readminmax - read min and max records
+--------------------------------------------------------------------------*/

void cmd_readminmax(int argc, char **argv) {
    
    Record max = recordLogger.getMax();
    Record min = recordLogger.getMin();

    printf("\nMin temp:% 2d C at %02d/%02d/%02d %02d:%02d:%02d\n",
    min.temp, min.timestamp.tm_mday, min.timestamp.tm_mon + 1,
    min.timestamp.tm_year + 1900, min.timestamp.tm_hour, 
    min.timestamp.tm_min, min.timestamp.tm_sec);

    printf("Max temp:% 2d C at %02d/%02d/%02d %02d:%02d:%02d",
    max.temp, max.timestamp.tm_mday, max.timestamp.tm_mon + 1, 
    max.timestamp.tm_year + 1900, max.timestamp.tm_hour, 
    max.timestamp.tm_min, max.timestamp.tm_sec);
}


/*-------------------------------------------------------------------------+
| Function: cmd_clearminmax - reset min and max records
+--------------------------------------------------------------------------*/
void cmd_clearminmax(int argc, char **argv) {
    recordLogger.clear();
}

/*-------------------------------------------------------------------------+
| Function: cmd_readparams - read PMON and TALA parameters
+--------------------------------------------------------------------------*/
void cmd_readparams  (int, char** ) {
    MUTEX_TAKE(ParamMutex)
    printf("\nMonitoring period (PMON) : %d\n", monitoring_period_PMON);
    printf("Alarm duration (TALA) : %d", alarm_duration_TALA);
    MUTEX_RETURN(ParamMutex)
}

/*-------------------------------------------------------------------------+
| Function: cmd_modmonperiod - set PMON
+--------------------------------------------------------------------------*/
void cmd_modmonperiod(int argc, char **argv) {
    MUTEX_TAKE(ParamMutex)
    if(validateInput(atoi(argv[1]), 0, 99, (int*)&monitoring_period_PMON))
        printf("\nValue out of range, clamping to %d", monitoring_period_PMON);

    if(monitoring_period_PMON == 0) {
        printf("\nPeriodic monitoring disabled");
        
        xTimerStop(SensorTimer, 1000);
    } else {
        xTimerChangePeriod(SensorTimer,
            pdMS_TO_TICKS(1000 * monitoring_period_PMON),
            1000);
        xTimerStart(SensorTimer, 1000); // start / reset timer in case it is off
    }

    MUTEX_RETURN(ParamMutex)
}

/*-------------------------------------------------------------------------+
| Function: cmd_modtimealarm - set TALA
+--------------------------------------------------------------------------*/
void cmd_modtimealarm(int argc, char **argv) {
    MUTEX_TAKE(ParamMutex)
    // We definitely do not want to ring for several days. Limiting to 10 minutes.
    if(validateInput(atoi(argv[1]), 0, 60, (int*)&alarm_duration_TALA))
        printf("\nValue out of range, clamping to %d", alarm_duration_TALA);
    MUTEX_RETURN(ParamMutex)
}

/*-------------------------------------------------------------------------+
| Function: cmd_readalarminfo - show alarm time, temp thresholds, and enable status
+--------------------------------------------------------------------------*/
void cmd_readalarminfo(int argc, char **argv) {
    MUTEX_TAKE(ParamMutex)
    printf("\nalarm clock set time: %02d:%02d:%02d\n",
           alarm_time.tm_hour, alarm_time.tm_min, alarm_time.tm_sec);
    printf("temperature thresholds: min% 2d max% 2d\n",
           low_threshold_TL, high_threshold_TH);
    printf("temperature alarm: %s\n", temp_alarm? "ON": "OFF");
    printf("alarm clock:       %s", alarm_clock? "ON": "OFF");
    MUTEX_RETURN(ParamMutex)
}

/*-------------------------------------------------------------------------+
| Function: cmd_setalarmclock - set alarm clock time
+--------------------------------------------------------------------------*/
void cmd_setalarmclock(int argc, char **argv) {
    MUTEX_TAKE(ParamMutex)

    int err = 0;
    err |= validateInput(
        (int)strtol(argv[1], NULL, 10), 0, 23, &alarm_time.tm_hour);
    err |= validateInput(
        (int)strtol(argv[2], NULL, 10), 0, 59, &alarm_time.tm_min);
    err |= validateInput(
        (int)strtol(argv[3], NULL, 10), 0, 59, &alarm_time.tm_sec);
    if(err) {
        printf("\nInput out of range, clamping to %02d:%02d:%02d",
               alarm_time.tm_hour, alarm_time.tm_min, alarm_time.tm_sec);
    }
    RTC::alarm(&alarmFunction, alarm_time); 

    MUTEX_RETURN(ParamMutex)
}

/*-------------------------------------------------------------------------+
| Function: cmd_setalarmtemp - set alarm temperature thresholds
+--------------------------------------------------------------------------*/
void cmd_setalarmtemp(int argc, char **argv) {
    MUTEX_TAKE(ParamMutex)
    int err = 0;
    err |= validateInput(
        atoi(argv[1]), 0, 50, (int*)&low_threshold_TL);
    err |= validateInput(
        atoi(argv[2]), 0, 50, (int*)&high_threshold_TH);
    if(err) {
        printf("\nInput out of range, clamping to :\n");
        printf("low limit% 2d, high limit% 2d",
               low_threshold_TL, high_threshold_TH);
    }
    MUTEX_RETURN(ParamMutex)
}

/*-------------------------------------------------------------------------+
| Function: cmd_alarmclocken - enable/disable alarm clock
+--------------------------------------------------------------------------*/
void cmd_alarmclocken(int argc, char **argv) {
    MUTEX_TAKE(ParamMutex)
    if(atoi(argv[1])) {
        alarm_clock = 1;
        printf("\nAlarm clock enabled.");
    }
    else {
        alarm_clock = 0;
        printf("\nAlarm clock disabled.");
    }
    MUTEX_RETURN(ParamMutex)
}

/*-------------------------------------------------------------------------+
| Function: cmd_tempalarmen - enable/disable temperature alarm
+--------------------------------------------------------------------------*/
void cmd_tempalarmen(int argc, char **argv) {
    MUTEX_TAKE(ParamMutex)
    if(atoi(argv[1])) {
        temp_alarm = 1;
        printf("\nTemperature alarm enabled.");
    }
    else {
        temp_alarm = 0;
        printf("\nTemperature alarm disabled.");
    }
    MUTEX_RETURN(ParamMutex)
}

/*-------------------------------------------------------------------------+
| Function: cmd_readtaskstate - check which tasks are enabled
+--------------------------------------------------------------------------*/
void cmd_readtaskstate(int argc, char **argv) {
    MUTEX_TAKE(StateMutex)
    printf("\nBubble Level %s\nHit Bit %s\nConfig Sound %s",
           bubble_level_bl? "ON":"OFF",
           hit_bit_hb? "ON":"OFF",
           config_sound_cs? "ON":"OFF");
    MUTEX_RETURN(StateMutex)
}

/*-------------------------------------------------------------------------+
| Function: cmd_bubblelevelen - enable/disable Bubble Level
+--------------------------------------------------------------------------*/
void cmd_bubblelevelen(int argc, char **argv) {
    MUTEX_TAKE(StateMutex)
    if(atoi(argv[1])) {
        bubble_level_bl = 1;
        vTaskResume(xTask_BubbleLevel);
        printf("\nBubble Level enabled.");
    }
    else {
        bubble_level_bl = 0;
        lcd.fillrect(95,0,127,31,0);
        vTaskSuspend(xTask_BubbleLevel);
        printf("\nBubble Level disabled.");
    }
    MUTEX_RETURN(StateMutex)
}

/*-------------------------------------------------------------------------+
| Function: cmd_hitbiten - enable/disable Hit Bit
+--------------------------------------------------------------------------*/
void cmd_hitbiten(int argc, char **argv) {
    MUTEX_TAKE(StateMutex)
    if(atoi(argv[1])) {
        hit_bit_hb = 1;
        bitGame.reset();
        vTaskResume(xTask_KillBitGame);
        printf("\nHit Bit enabled.");
    }
    else {
        hit_bit_hb = 0;
        vTaskSuspend(xTask_KillBitGame);
        bitGame.off();
        printf("\nHit Bit disabled.");
    }
    MUTEX_RETURN(StateMutex)
}

    


/*-------------------------------------------------------------------------+
| Function: cmd_configsounden - enable/disable Config Sound
+--------------------------------------------------------------------------*/
void cmd_configsounden(int argc, char **argv) {
    MUTEX_TAKE(StateMutex)
    if(atoi(argv[1])) {
        config_sound_cs = 1;
        printf("\nConfig Sound enabled.");
    }
    else {
        config_sound_cs = 0;
        printf("\nConfig Sound disabled.");
    }
    MUTEX_RETURN(StateMutex)
}

//#endif //notdef

