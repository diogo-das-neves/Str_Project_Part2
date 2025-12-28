#include "mbed.h"
#include "FreeRTOS.h"
#include "portmacro.h"
#include "projdefs.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "extras.h"

#include "LM75B.h"
#include "C12832.h"
#include "RTC.h"
#include "MMA7660.h"


volatile int TL = 10;
volatile int TM = 25;
volatile int PMON = 5;
volatile int TALA = 10;

SemaphoreHandle_t AlarmMutex;#include "task.h"
SemaphoreHandle_t ClockMutex;
SemaphoreHandle_t RecordMutex;

TaskHandle_t xTask_temp;
AnalogIn pot1(p19);
AnalogIn pot2(p20);

DigitalOut led1(LED1);
DigitalOut led2(LED2);
Serial pc(USBTX, USBRX);

PwmOut r(p23);
PwmOut g(p24);
PwmOut b(p25);

LM75B sensor(p28,p27); // temp sensor
C12832 lcd(p5, p7, p6, p8, p11); // lcd
MMA7660 MMA(p28, p27); // I2C accelerometer
PwmOut spkr(p26); //buzzer


QueueHandle_t xQueue;
QueueHandle_t xQueue2;

extern void monitor(void); //shared vars have to be protected
extern float sensor_read;
volatile bool alarm;
volatile float Period;
volatile float DutyCycle;
Record maxtemp;
Record mintemp;


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
    float x = 0;
    float y = 0;
    for(;;){
        x = (x + MMA.x() * 16.0)/2.0;
        y = (y -(MMA.y() * 16.0))/2.0;
        lcd.fillcircle(x+63, y+15, 3, 1); //draw bubble
        lcd.circle(63, 15, 8, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
        lcd.fillcircle(x+63, y+15, 3, 0); //erase bubble
    }
}
void vTask_Alarm(void *pvParameters){
    BaseType_t xStatus;
    int32_t AlarmTrigger;

    for(;;){
        if(xSemaphoreTake(AlarmMutex, 200)){
            if(alarm){
                spkr.period(Period);
                spkr = DutyCycle;
            } else {
                spkr = 0.0f;
            }
            xSemaphoreGive(AlarmMutex);
        }
    vTaskDelay(pdMS_TO_TICKS(100));
    }
}
void vTask_Pot1(void *pvParameters){
    BaseType_t xStatus;
    float f;
    for(;;){
        f = pot1.read()*5000;
        if(f <= 0){f = 0.01;}
        if(xSemaphoreTake(AlarmMutex, 200))
            Period = 1/f;
            xSemaphoreGive(AlarmMutex);
        vTaskDelay(pdMS_TO_TICKS(200));

        }
    }
void vTask_Pot2(void *pvParameters){
    BaseType_t xStatus;
    for(;;){
        if(xSemaphoreTake(AlarmMutex, 200))
            DutyCycle = pot2.read();
            xSemaphoreGive(AlarmMutex);
        vTaskDelay(pdMS_TO_TICKS(200));

        }
    }
    
void vTask_temp(void *pvParameters){
    BaseType_t xStatus;
    float sensor_read;
    sensor.open();
    for(;;){
        if(PMON > 0){
            sensor_read = sensor.temp();
            xStatus = xQueueSend(xQueue2, &sensor_read, 0);
            vTaskDelay(pdMS_TO_TICKS(PMON*1000));//5 secs
        }else {
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            sensor_read = sensor.temp();
            xStatus = xQueueSend(xQueue2, &sensor_read, 0);
            }

    }
}

void vTask_LCD(void *pvParameters){
    BaseType_t xStatus;
    float sensor_read;
    time_t t;
    tm tm;
    for(;;){
        xStatus = xQueueReceive(xQueue2, &sensor_read, 1000);
        if(xStatus==pdPASS){
            if(xSemaphoreTake(ClockMutex,100))
                time(&t);
                localtime_r(&t, &tm);
                lcd.locate(0,0); //3
                lcd.printf("%d:%d:%d",tm.tm_hour,tm.tm_min,tm.tm_sec);
                xSemaphoreGive(ClockMutex);
            lcd.locate(0,11); //13
            lcd.printf("A: C T");
            lcd.locate(0,22); //26
            lcd.printf("T(C) = %.3f\n", sensor_read);
        }
    }
}
void vTask_records(void *pvParameters){
    BaseType_t xStatus;
    float sensor_read;
    time_t t;
    tm tm;
    for(;;){
        xStatus = xQueueReceive(xQueue2, &sensor_read, 1000);
        if(xStatus==pdPASS){
            if(sensor_read > maxtemp.temp){
                if(xSemaphoreTake(ClockMutex,200)){
                    time(&t);
                    localtime_r(&t, &tm);
                    maxtemp.temp = sensor_read;
                    maxtemp.timestamp.tm_sec = tm.tm_sec;
                    maxtemp.timestamp.tm_min = tm.tm_min;
                    maxtemp.timestamp.tm_hour = tm.tm_hour;
                    xSemaphoreGive(ClockMutex);
                    }
            }
            if(sensor_read < mintemp.temp){
                if(xSemaphoreTake(ClockMutex,200)){
                    time(&t);
                    localtime_r(&t, &tm);
                    mintemp.temp = sensor_read;
                    mintemp.timestamp.tm_sec = tm.tm_sec;
                    mintemp.timestamp.tm_min = tm.tm_min;
                    mintemp.timestamp.tm_hour = tm.tm_hour;
                    xSemaphoreGive(ClockMutex);
                    }
            }
            
        }
    }
}
void vTask_Temp_Light_Alarm(void *pvParamaters){
    BaseType_t xStatus;
    float sensor_read;
    for(;;){
        xStatus = xQueueReceive(xQueue2, &sensor_read, 1000);
        if (sensor_read >= 25){
            r = 0.8;
            g = 1;
            b = 1;
            if(xSemaphoreTake(AlarmMutex, 500))
                alarm = true;
            xSemaphoreGive(AlarmMutex);
        }
        else if(sensor_read <= 23){
            r = 1;
            g = 1;
            b = 0.8;
            if(xSemaphoreTake(AlarmMutex, 500))
                alarm = true;
            xSemaphoreGive(AlarmMutex);
        }
        else{
            r = 1;
            g = 0.8;
            b = 1;
            if(xSemaphoreTake(AlarmMutex, 500))
                alarm = false;
            xSemaphoreGive(AlarmMutex);
        }
    }
}

int main( void ) {
    /* Perform any hardware setup necessary. */
//    prvSetupHardware();
    maxtemp.temp = 0;
    mintemp.temp = 50;
    set_time(0);
    pc.baud(115200);

    AlarmMutex = xSemaphoreCreateMutex();
    ClockMutex = xSemaphoreCreateMutex();
//    printf("Hello from mbed -- FreeRTOS / cmd\n");

    /* --- APPLICATION TASKS CAN BE CREATED HERE --- */

    xQueue = xQueueCreate( 4, sizeof( int32_t ) );
    xQueue2 = xQueueCreate( 4, sizeof( float ) );

    //xTaskCreate( vTask1, "Task 1", 2*configMINIMAL_STACK_SIZE, NULL, 1, NULL );
    //xTaskCreate( vTask2, "Task 2", 2*configMINIMAL_STACK_SIZE, NULL, 2, NULL );
    xTaskCreate( vTask_temp, "Temp Task", 2*configMINIMAL_STACK_SIZE, NULL, 1, &xTask_temp );
    xTaskCreate( vTask_LCD, "LCD Task", 2*configMINIMAL_STACK_SIZE, NULL, 2, NULL );
    xTaskCreate( vTask_Temp_Light_Alarm, "TempAlarm Task", 2*configMINIMAL_STACK_SIZE, NULL, 2, NULL );
    xTaskCreate( vTask_records, "TempRecords Task", 2*configMINIMAL_STACK_SIZE, NULL, 2, NULL );

    //xTaskCreate( vTask_MCU, "LCD Task", 2*configMINIMAL_STACK_SIZE, NULL, 2, NULL );
    /* Start the created tasks running. */
    vTaskStartScheduler();

    /* Execution will only reach here if there was insufficient heap to
    start the scheduler. */
    for( ;; );
    return 0;
}
