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

enum{
    TL = 10,
    TM = 25,
    PMON = 5
};

struct Record{
    uint8_t temp;
    tm timestamp;
};
typedef Record Record;

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

enum{
    TL = 10,
    TM = 25,
    PMON = 5
};

struct Record{
    uint8_t temp;
    tm timestamp;
};
typedef Record Record;


DigitalOut led1(LED1);
DigitalOut led2(LED2);
Serial pc(USBTX, USBRX);

PwmOut r(p23);
PwmOut g(p24);
PwmOut b(p25);

LM75B sensor(p28,p27); // temp sensor
C12832 lcd(p5, p7, p6, p8, p11); // lcd
MMA7660 MMA(p28, p27); // I2C accelerometer



QueueHandle_t xQueue;

extern void monitor(void);
extern float sensor_read;
volatile uint8_t seconds;
volatile uint8_t minutes;
volatile uint8_t hours;
Record maxtemp;
Record mintemp;

maxtemp.temp = 0;
mintemp.temp = 50;
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

void vTask_LCD(void *pvParameters){
    BaseType_t xStatus;
    float sensor_read;
    for(;;){
        xStatus = xQueueReceive(xQueue, &sensor_read, 1000);
        if(xStatus==pdPASS){
            lcd.locate(0,0); //3
            lcd.printf("hh:mm:ss");
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
    for(;;){
        xStatus = xQueueReceive(xQueue, &sensor_read, 1000);
        if(xStatus==pdPASS){
            if(sensor_read > maxtemp.temp){
                maxtemp.temp = sensor_read;
                maxtemp.timestamp.seconds = seconds;
                maxtemp.timestamp.minutes = minutes;
                maxtemp.timestamp.minutes = hours;
            }
            if(sensor_read < mintemp.temp){
                mintemp.temp = sensor_read;
                mintemp.timestamp.seconds = seconds;
                mintemp.timestamp.minutes = minutes;
                mintemp.timestamp.minutes = hours;
            }
            
        }
    }
}
void vTask_Temp_Light_Alarm(void *pvParamaters){
    BaseType_t xStatus;
    float sensor_read;
    for(;;){
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
    }
}

void alarmFunction( void )
{
    seconds++;
    if(seconds >= 60){
        minutes++;
        seconds-=60;
    }
    if(minutes >= 60){
        hours++:
        minutes-=60;
    }
}
int main( void ) {
    /* Perform any hardware setup necessary. */
//    prvSetupHardware();
    set_time(0);
    if(PMON != 0)
        RTC::attach(&alarmFunction, RTC::Second);
    pc.baud(115200);

//    printf("Hello from mbed -- FreeRTOS / cmd\n");

    /* --- APPLICATION TASKS CAN BE CREATED HERE --- */

    xQueue = xQueueCreate( 4, sizeof( int32_t ) );
    
    //xTaskCreate( vTask1, "Task 1", 2*configMINIMAL_STACK_SIZE, NULL, 1, NULL );
    //xTaskCreate( vTask2, "Task 2", 2*configMINIMAL_STACK_SIZE, NULL, 2, NULL );
    xTaskCreate( vTask_temp, "Temp Task", 2*configMINIMAL_STACK_SIZE, NULL, 1, NULL );
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

DigitalOut led1(LED1);
DigitalOut led2(LED2);
Serial pc(USBTX, USBRX);

PwmOut r(p23);
PwmOut g(p24);
PwmOut b(p25);

LM75B sensor(p28,p27); // temp sensor
C12832 lcd(p5, p7, p6, p8, p11); // lcd
MMA7660 MMA(p28, p27); // I2C accelerometer



QueueHandle_t xQueue;

extern void monitor(void);
extern float sensor_read;
volatile uint8_t seconds;
volatile uint8_t minutes;
volatile uint8_t hours;
Record maxtemp;
Record mintemp;

maxtemp.temp = 0;
mintemp.temp = 50;
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

void vTask_LCD(void *pvParameters){
    BaseType_t xStatus;
    float sensor_read;
    for(;;){
        xStatus = xQueueReceive(xQueue, &sensor_read, 1000);
        if(xStatus==pdPASS){
            lcd.locate(0,0); //3
            lcd.printf("hh:mm:ss");
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
    for(;;){
        xStatus = xQueueReceive(xQueue, &sensor_read, 1000);
        if(xStatus==pdPASS){
            if(sensor_read > maxtemp.temp){
                maxtemp.temp = sensor_read;
                maxtemp.timestamp.seconds = seconds;
                maxtemp.timestamp.minutes = minutes;
                maxtemp.timestamp.minutes = hours;
            }
            if(sensor_read < mintemp.temp){
                mintemp.temp = sensor_read;
                mintemp.timestamp.seconds = seconds;
                mintemp.timestamp.minutes = minutes;
                mintemp.timestamp.minutes = hours;
            }
            
        }
    }
}
void vTask_Temp_Light_Alarm(void *pvParamaters){
    BaseType_t xStatus;
    float sensor_read;
    for(;;){
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
    }
}

void alarmFunction( void )
{
    seconds++;
    if(seconds >= 60){
        minutes++;
        seconds-=60;
    }
    if(minutes >= 60){
        hours++:
        minutes-=60;
    }
}
int main( void ) {
    /* Perform any hardware setup necessary. */
//    prvSetupHardware();
    set_time(0);
    if(PMON != 0)
        RTC::attach(&alarmFunction, RTC::Second);
    pc.baud(115200);

//    printf("Hello from mbed -- FreeRTOS / cmd\n");

    /* --- APPLICATION TASKS CAN BE CREATED HERE --- */

    xQueue = xQueueCreate( 4, sizeof( int32_t ) );
    
    //xTaskCreate( vTask1, "Task 1", 2*configMINIMAL_STACK_SIZE, NULL, 1, NULL );
    //xTaskCreate( vTask2, "Task 2", 2*configMINIMAL_STACK_SIZE, NULL, 2, NULL );
    xTaskCreate( vTask_temp, "Temp Task", 2*configMINIMAL_STACK_SIZE, NULL, 1, NULL );
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
