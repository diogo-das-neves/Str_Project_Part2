#include "mbed.h"
#include "FreeRTOS.h"
#include "portmacro.h"
#include "projdefs.h"
#include "task.h"
#include "queue.h"


#include "LM75B.h"
#include "C12832.h"
#include "RTC.h"
#include "MMA7660.h"


<<<<<<< Updated upstream
DigitalOut led1(LED1);
DigitalOut led2(LED2);
=======
#include "led.h"
#include "RGBled.h"
#include "RecordManager.h"


volatile int low_threshold_TL = 10;
volatile int high_threshold_TH = 25;
volatile int monitoring_period_PMON = 5;
volatile int alarm_duration_TALA = 10;

SemaphoreHandle_t AlarmMutex;
SemaphoreHandle_t ClockMutex;
SemaphoreHandle_t RecordMutex;
SemaphoreHandle_t ParamMutex;
SemaphoreHandle_t StateMutex;

TaskHandle_t xTask_temp;
TaskHandle_t xTask_Alarm;
TaskHandle_t xTask_KillBitGame;

TimerHandle_t SensorTimer;

AnalogIn pot1(p19);
AnalogIn pot2(p20);

>>>>>>> Stashed changes
Serial pc(USBTX, USBRX);

PwmOut r(p23);
PwmOut g(p24);
PwmOut b(p25);

LM75B sensor(p28,p27); // temp sensor
C12832 lcd(p5, p7, p6, p8, p11); // lcd
MMA7660 MMA(p28, p27); // I2C accelerometer

DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);
DigitalOut led4(LED4);
DigitalIn pb(p14); // joystick

RGBLed tempLed(p23, p24, p25);

QueueHandle_t xQueue;

extern void monitor(void);
extern float sensor_read;
<<<<<<< Updated upstream
=======
volatile bool alarm; // #TODO replace this with alarm_clock | temp_alarm, since they are separate
volatile bool alarm_clock = false;
volatile bool temp_alarm = false;
volatile float Period;
volatile float DutyCycle;

RecordManager recordLogger;
>>>>>>> Stashed changes


/*-------------------------------------------------------------------------+
| Function: my_fgets        (called from my_getline / monitor) 
+--------------------------------------------------------------------------*/ 
char* my_fgets (char* ln, int sz, FILE* f)
{
//  fgets(line, MAX_LINE, stdin);
//  pc.gets(line, MAX_LINE);
  int i; char c;
  for(i=0; i<sz-1; i++) {
      c = pc.getc();
      ln[i] = c;
      if ((c == '\n') || (c == '\r')) break;
  }
  ln[i] = '\0';

  return ln;
}

void vTask1( void *pvParameters ) {
int32_t lValueToSend;
BaseType_t xStatus;
    led1 = 1;
    for( ;; ) {
        lValueToSend = 201;
        xStatus = xQueueSend( xQueue, &lValueToSend, 0 );
        monitor(); //does not return
        led1 = !led1;
    }
}

<<<<<<< Updated upstream
void vTask2( void *pvParameters ) {
int32_t lReceivedValue;
BaseType_t xStatus;

    led2 = 1;
    printf("Hello from mbed -- FreeRTOS / cmd\n");
    for( ;; ) {
//        vTaskDelay( 1000 );
        xStatus = xQueueReceive( xQueue, &lReceivedValue, 1000 );
        if( xStatus == pdPASS ) {
            printf( "Received = %d", lReceivedValue );
        }
        led2 = !led2;
    }
}

void vTask_MCU(void *pvParameters){
    BaseType_t xStatus;
=======
/*------------------+
| Bubble Level Task
+-------------------*/ 

void vTask_BubbleLevel(void *pvParameters){
>>>>>>> Stashed changes
    float x = 0;
    float y = 0;
    for(;;){
        x = (x + MMA.x() * 16.0)/2.0;
        y = (y -(MMA.y() * 16.0))/2.0;
        lcd.fillcircle(x+63, y+15, 3, 1); //draw bubble
        lcd.circle(63, 15, 8, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
<<<<<<< Updated upstream
        lcd.fillcircle(x+63, y+15, 3, 0); //erase bubble
    }
}


=======
        lcd.fillcircle(x+111, y+15, 3, 0); //erase bubble
    }
}

/*------------+
| Alarm Task
+-------------*/ 

void vTask_Alarm(void *pvParameters){

    for(;;){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        MUTEX_TAKE(AlarmMutex)
        if(alarm){
            spkr.period(Period);
            spkr = DutyCycle;
        } else {
            spkr = 0.0f;
        }
        MUTEX_RETURN(AlarmMutex)
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/*------------------------+
| Potentiometers Tasks
+-------------------------*/ 

void vTask_Pot1(void *pvParameters){
    float f;
    for(;;){
        f = pot1.read()*5000;
        if(f <= 0){f = 0.01;}
        MUTEX_TAKE(AlarmMutex)
        Period = 1/f;
        MUTEX_RETURN(AlarmMutex)
        vTaskDelay(pdMS_TO_TICKS(200));

    }
}

void vTask_Pot2(void *pvParameters){
    for(;;){
        MUTEX_TAKE(AlarmMutex)
        DutyCycle = pot2.read();
        MUTEX_RETURN(AlarmMutex)
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}


/*-------------------+
| Temperature Task
+--------------------*/     
>>>>>>> Stashed changes
void vTask_temp(void *pvParameters){
    BaseType_t xStatus;
    float sensor_read;
    for(;;){
        if (sensor.open()) {
            sensor_read = sensor.temp();
            xStatus = xQueueSend(xQueue, &sensor_read, 0);
        }
    }
}

/*-----------+
| LCD Task
+------------*/ 
void vTask_LCD(void *pvParameters){
    BaseType_t xStatus;
    float sensor_read;
    for(;;){
<<<<<<< Updated upstream
        xStatus = xQueueReceive(xQueue, &sensor_read, 1000);
        if(xStatus==pdPASS){
            lcd.locate(0,0); //3
            lcd.printf("hh:mm:ss");
            lcd.locate(0,11); //13
            lcd.printf("A: C T");
            lcd.locate(0,22); //26
            lcd.printf("T(C) = %.3f\n", sensor_read);
=======
        MUTEX_TAKE(ClockMutex)
        time(&t);
        MUTEX_RETURN(ClockMutex)

        MUTEX_TAKE(TempMutex)
        sensor_read = temperature; // cache temperature
        MUTEX_RETURN(TempMutex)

        localtime_r(&t, &tm);
        //lcd.fillrect(0,0,94,32,0); // clear framebuffer
        lcd.locate(0,0); //3
        lcd.printf("%02d:%02d:%02d",tm.tm_hour,tm.tm_min,tm.tm_sec);
        lcd.locate(0,11); //13
        lcd.printf("A: C T");
        lcd.locate(0,22); //26

        lcd.printf("T(C) =%7.3f\n", sensor_read);

        vTaskDelay(pdMS_TO_TICKS(33)); //30Hz LCD
    }
}


/*--------------+
| Records Task with OOP
+---------------*/ 

void vTask_records(void *pvParameters){
    float sensor_read;
    for(;;){
        MUTEX_TAKE(TempMutex)
        sensor_read = temperature; // cache temperature
        MUTEX_RETURN(TempMutex)

        /*MUTEX_TAKE(ClockMutex)
        time(&t);
        MUTEX_RETURN(ClockMutex)
        localtime_r(&t, &tm);
        */
        recordLogger.update(sensor_read);

        vTaskDelay(pdMS_TO_TICKS(500));
        //this catches even the fastest periodic monitoring
    }
}


/*--------------+
| Records Task
+---------------*/ 
/*
void vTask_records(void *pvParameters){
    float sensor_read;
    time_t t;
    tm tm;
    for(;;){
        MUTEX_TAKE(TempMutex)
        sensor_read = temperature; // cache temperature
        MUTEX_RETURN(TempMutex)

        MUTEX_TAKE(ClockMutex)
        time(&t);
        MUTEX_RETURN(ClockMutex)
        localtime_r(&t, &tm);

        if(sensor_read > maxtemp.temp){
            maxtemp.temp = sensor_read;
            maxtemp.timestamp.tm_sec = tm.tm_sec;
            maxtemp.timestamp.tm_min = tm.tm_min;
            maxtemp.timestamp.tm_hour = tm.tm_hour;
            maxtemp.timestamp.tm_mday = tm.tm_mday;
            maxtemp.timestamp.tm_mon = tm.tm_mon;
            maxtemp.timestamp.tm_year = tm.tm_year;
>>>>>>> Stashed changes
        }
    }
}
*/
/*-------------------------+
| Alarm Temperature Task
+--------------------------*/ 

void vTask_Temp_Light_Alarm(void *pvParamaters){
    BaseType_t xStatus;
    float sensor_read;
    for(;;){
<<<<<<< Updated upstream
        xStatus = xQueueReceive(xQueue, &sensor_read, 1000);
        if (sensor_read >= 25){
            r = 0.8;
            g = 1;
            b = 1;
        }
        else if(sensor_read <= 23){
            r = 1;
            g = 1;
            b = 0.8;
        }
        else{
            r = 1;
            g = 0.8;
            b = 1;
        }
=======
        const float LED_BRIGHTNESS = 0.5;
        MUTEX_TAKE(TempMutex)
        sensor_read = temperature; // cache temperature
        MUTEX_RETURN(TempMutex)

        if (sensor_read >= (float)high_threshold_TH){
            tempLed.hsv(0.0, 1.0, LED_BRIGHTNESS);
            if(alarm && xSemaphoreTake(AlarmMutex, 500))
                xTaskNotify(xTask_Alarm, 0,eNoAction);
            xSemaphoreGive(AlarmMutex);
        }
        else if(sensor_read <= (float)low_threshold_TL){
            tempLed.hsv(240.0, 1.0, LED_BRIGHTNESS);
            if(alarm && xSemaphoreTake(AlarmMutex, 500))
                xTaskNotify(xTask_Alarm, 0,eNoAction);
            xSemaphoreGive(AlarmMutex);
        }
        else{
            float H = (1.0 - (sensor_read - (float)low_threshold_TL) / (float)(high_threshold_TH - low_threshold_TL)) * 240.0;

            tempLed.hsv(H, 1.0, LED_BRIGHTNESS);
        }
        vTaskDelay(pdMS_TO_TICKS(33)); // 30 Hz
>>>>>>> Stashed changes
    }
}


/*-------------------------------------------------------------------------+
| Start of Kill Bit Game section
+--------------------------------------------------------------------------*/ 
// Kill Bit helper function
void LEDS(int number) {
  led1 = (number)&0x01;
  led2 = (number >> 1) & 0x01;
  led3 = (number >> 2) & 0x01;
  led4 = (number >> 3) & 0x01;
}

void vTask_KillBitGame(void *pvParameters) {
  unsigned int value = 0x12;
  spkr.period(1.0 / 2000.0);
  for (;;) {
    // Handle Pushbutton XOR (Check if pb is a DigitalIn)
    value = value ^ pb;
    if (value == 0) {
      // Alarm Sequence
      for (int i = 0; i < 5; ++i) {
        spkr = 0.5;
        LEDS(0x0F);
        vTaskDelay(pdMS_TO_TICKS(500)); // wait(.5)
        LEDS(0);
        spkr = 0.0;
        vTaskDelay(pdMS_TO_TICKS(250)); // wait(.25)
      }
      value = 0x12; // Reset value
    }
    // Bitwise rotation logic
    value = ((value & 0x01) << 3) | (value >> 1);
    LEDS(value);
    // Periodic delay
    vTaskDelay(pdMS_TO_TICKS(250));
  }
}
/*-------------------------------------------------------------------------+
| End of Kill Bit Game section
+--------------------------------------------------------------------------*/ 


int main( void ) {
    /* Perform any hardware setup necessary. */
//    prvSetupHardware();
<<<<<<< Updated upstream

=======
    //maxtemp.temp = 0;
    //mintemp.temp = 50;
    set_time(0);
>>>>>>> Stashed changes
    pc.baud(115200);

//    printf("Hello from mbed -- FreeRTOS / cmd\n");

    /* --- APPLICATION TASKS CAN BE CREATED HERE --- */

    xQueue = xQueueCreate( 4, sizeof( int32_t ) );
    
    //xTaskCreate( vTask1, "Task 1", 2*configMINIMAL_STACK_SIZE, NULL, 1, NULL );
    //xTaskCreate( vTask2, "Task 2", 2*configMINIMAL_STACK_SIZE, NULL, 2, NULL );
    xTaskCreate( vTask_temp, "Temp Task", 2*configMINIMAL_STACK_SIZE, NULL, 1, NULL );
    xTaskCreate( vTask_LCD, "LCD Task", 2*configMINIMAL_STACK_SIZE, NULL, 2, NULL );
    xTaskCreate( vTask_Temp_Light_Alarm, "TempAlarm Task", 2*configMINIMAL_STACK_SIZE, NULL, 2, NULL );
<<<<<<< Updated upstream
    //xTaskCreate( vTask_MCU, "LCD Task", 2*configMINIMAL_STACK_SIZE, NULL, 2, NULL );
=======
    xTaskCreate( vTask_records, "TempRecords Task", 2*configMINIMAL_STACK_SIZE, NULL, 2, NULL );
    xTaskCreate( vTask_KillBitGame, "KillBitGame", 2*configMINIMAL_STACK_SIZE, NULL, 2, &xTask_KillBitGame );
    vTaskSuspend(xTask_KillBitGame);

    //xTaskCreate( vTask_BubbleLevel, "Bubble Level Task", 2*configMINIMAL_STACK_SIZE, NULL, 2, NULL );
>>>>>>> Stashed changes
    /* Start the created tasks running. */
    vTaskStartScheduler();

    /* Execution will only reach here if there was insufficient heap to
    start the scheduler. */
    for( ;; );
    return 0;
}
