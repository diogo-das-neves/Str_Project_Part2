#include "mbed.h"
#include "FreeRTOS.h"
#include "portmacro.h"
#include "projdefs.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "extras.h"
#include "timers.h"

#include "LM75B.h"
#include "C12832.h"
#include "RTC.h"
#include "MMA7660.h"


volatile int low_threshold_TL = 10;
volatile int high_threshold_TH = 25;
volatile int monitoring_period_PMON = 5;
volatile int alarm_duration_TALA = 10;
volatile bool alarm_clock = 1;
volatile bool temp_alarm = 1;

SemaphoreHandle_t AlarmMutex;
SemaphoreHandle_t ClockMutex;
SemaphoreHandle_t RecordMutex;
SemaphoreHandle_t ParamMutex;
SemaphoreHandle_t StateMutex;

TaskHandle_t xTask_temp;
TaskHandle_t xTask_TempLight;
TaskHandle_t xTask_Records;
TaskHandle_t xTask_AlarmClock;
TaskHandle_t xTask_AlarmTemp;
TaskHandle_t xTask_Bubble;
TaskHandle_t xTask_Pot1;
TaskHandle_t xTask_Pot2;
TaskHandle_t xTask_KillBitGame;



TimerHandle_t SensorTimer;

AnalogIn pot1(p19);
AnalogIn pot2(p20);



DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);
DigitalOut led4(LED4);
DigitalIn pb(p14); // joystick

RGBLed tempLed(p23, p24, p25);

Serial pc(USBTX, USBRX);
LM75B sensor(p28,p27); // temp sensor
C12832 lcd(p5, p7, p6, p8, p11); // lcd
MMA7660 MMA(p28, p27); // I2C accelerometer
PwmOut spkr(p26); //buzzer


QueueHandle_t xQueue;

SemaphoreHandle_t xMeasureTempSemaphore;
SemaphoreHandle_t TempMutex;
volatile float temperature; 

extern void monitor(void); //shared vars have to be protected
extern float sensor_read;
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

void vTask_Serial( void *pvParameters ) {
    monitor(); //does not return
}

void vTask_BubbleLevel(void *pvParameters){
    float x = 0;
    float y = 0;
    for(;;){
        x = (x + MMA.x() * 16.0)/2.0;
        y = (y -(MMA.y() * 16.0))/2.0;
        lcd.fillcircle(x+111, y+15, 3, 1); //draw bubble
        lcd.circle(111, 15, 8, 1);
        lcd.line(95,0,95,31,1); // draw margin line
        vTaskDelay(pdMS_TO_TICKS(100));
        lcd.fillcircle(x+111, y+15, 3, 0); //erase bubble
    }
}
void vTask_AlarmTemp(void *pvParameters){

    for(;;){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        MUTEX_TAKE(AlarmMutex)
        spkr.period(Period);
        spkr = DutyCycle;
        MUTEX_RETURN(AlarmMutex)
        
        vTaskDelay(pdMS_TO_TICKS(alarm_duration_TALA * 1000));
        MUTEX_TAKE(AlarmMutex)
        spkr = 0.0f;
        MUTEX_RETURN(AlarmMutex)
    }
}
void vTask_AlarmClock(void *pvParameters){

    for(;;){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        MUTEX_TAKE(AlarmMutex)
        spkr.period(Period);
        spkr = DutyCycle;
        MUTEX_RETURN(AlarmMutex)

        vTaskDelay(pdMS_TO_TICKS(alarm_duration_TALA * 1000));
        MUTEX_TAKE(AlarmMutex)
        spkr = 0.0f;
        MUTEX_RETURN(AlarmMutex)
    }
}
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
    
void vTask_temp(void *pvParameters){
    sensor.open();
    for(;;){
        if(xSemaphoreTake(xMeasureTempSemaphore, portMAX_DELAY)) {
            MUTEX_TAKE(TempMutex)
            temperature = sensor.temp();
            MUTEX_RETURN(TempMutex)
        }
    }
}
static void TempTimerCallback(TimerHandle_t xTimer) {
    xSemaphoreGive(xMeasureTempSemaphore);
}


void vTask_LCD(void *pvParameters){
    time_t t;
    tm tm;
    float sensor_read;
    for(;;){
        MUTEX_TAKE(ClockMutex)
        time(&t);
        MUTEX_RETURN(ClockMutex)

        MUTEX_TAKE(TempMutex)
        sensor_read = temperature; // cache temperature
        MUTEX_RETURN(TempMutex)

        localtime_r(&t, &tm);
        lcd.locate(0,0); //3
        //lcd.fillrect(0,0,94,32,0); // clear framebuffer
        lcd.printf("%02d:%02d:%02d",tm.tm_hour,tm.tm_min,tm.tm_sec);
        lcd.locate(0,11); //13
        lcd.printf("A: C T");
        lcd.locate(0,22); //26

        lcd.printf("T(C) =%7.3f\n", sensor_read);

        vTaskDelay(pdMS_TO_TICKS(33)); //30Hz LCD
    }
}

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
        }
        if(sensor_read < mintemp.temp){
            mintemp.temp = sensor_read;
            mintemp.timestamp.tm_sec = tm.tm_sec;
            mintemp.timestamp.tm_min = tm.tm_min;
            mintemp.timestamp.tm_hour = tm.tm_hour;
            mintemp.timestamp.tm_mday = tm.tm_mday;
            mintemp.timestamp.tm_mon = tm.tm_mon;
            mintemp.timestamp.tm_year = tm.tm_year;
        }
      
        vTaskDelay(pdMS_TO_TICKS(500));
        //this catches even the fastest periodic monitoring
    }
}

void vTask_Temp_Light_Alarm(void *pvParamaters){
    float sensor_read;
    for(;;){
        const float LED_BRIGHTNESS = 0.2;
        MUTEX_TAKE(TempMutex)
        sensor_read = temperature; // cache temperature
        MUTEX_RETURN(TempMutex)

        if (sensor_read >= (float)high_threshold_TH){
            hsvLED(0.0, 1.0, LED_BRIGHTNESS);
            if(alarm && xSemaphoreTake(AlarmMutex, 500))
                xTaskNotify(xTask_AlarmTemp, 0,eNoAction);
            xSemaphoreGive(AlarmMutex);
        }
        else if(sensor_read <= (float)low_threshold_TL){
            hsvLED(240.0, 1.0, LED_BRIGHTNESS);
            if(alarm && xSemaphoreTake(AlarmMutex, 500))
                xTaskNotify(xTask_AlarmTemp, 0,eNoAction);
            xSemaphoreGive(AlarmMutex);
        }
        else{
            float H = (1.0 - (sensor_read - (float)low_threshold_TL) / ((float)high_threshold_TH - (float)low_threshold_TL)) * 240.0;

            hsvLED(H, 1.0, LED_BRIGHTNESS);
        }

        vTaskDelay(pdMS_TO_TICKS(33)); // 30 Hz
    }
}

void alarmFunction(void){
    xTaskNotify(xTask_AlarmClock, 0,eNoAction);
}

void LEDS(int number) {
  led4 = (number)&0x01;
  led3 = (number >> 1) & 0x01;
  led2 = (number >> 2) & 0x01;
  led1 = (number >> 3) & 0x01;
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
int main( void ) {
    /* Perform any hardware setup necessary. */
//    prvSetupHardware();
    maxtemp.temp = 0;
    mintemp.temp = 50;
    set_time(0);
    pc.baud(115200);

    r.period(0.00005); // some people (including me) get headaches from PWM frequencies
    g.period(0.00005); // under a few kHz. Setting to 20kHz. Kat
    b.period(0.00005);

    AlarmMutex = xSemaphoreCreateMutex();
    ClockMutex = xSemaphoreCreateMutex();
    RecordMutex = xSemaphoreCreateMutex();
    StateMutex = xSemaphoreCreateMutex();
    ParamMutex = xSemaphoreCreateMutex();
    TempMutex = xSemaphoreCreateMutex();
//    printf("Hello from mbed -- FreeRTOS / cmd\n");

    /* --- APPLICATION TASKS CAN BE CREATED HERE --- */

    xQueue = xQueueCreate( 4, sizeof( int32_t ) );
    vSemaphoreCreateBinary(xMeasureTempSemaphore);
    xSemaphoreGive(xMeasureTempSemaphore);


    SensorTimer = xTimerCreate(
        "SensorTimer",
         pdMS_TO_TICKS(1000 * monitoring_period_PMON),
         pdTRUE,
         NULL,
         TempTimerCallback);

    xTaskCreate( vTask_Serial, "SerialComms Task", 2*configMINIMAL_STACK_SIZE, NULL, 1, NULL );
    xTaskCreate( vTask_AlarmClock, "Alarm  clock Task", 2*configMINIMAL_STACK_SIZE, NULL, 2, &xTask_AlarmClock );
    xTaskCreate( vTask_AlarmTemp, "Alarm temp Task", 2*configMINIMAL_STACK_SIZE, NULL, 2, &xTask_AlarmTemp );
    xTaskCreate( vTask_temp, "Temp Task", 2*configMINIMAL_STACK_SIZE, NULL, 5, &xTask_temp );
    xTaskCreate( vTask_LCD, "LCD Task", 2*configMINIMAL_STACK_SIZE, NULL, 2, NULL );
    xTaskCreate( vTask_Temp_Light_Alarm, "TempAlarm Task", 2*configMINIMAL_STACK_SIZE, NULL, 2, &xTask_TempLight );
    xTaskCreate( vTask_records, "TempRecords Task", 2*configMINIMAL_STACK_SIZE, NULL, 2, NULL);
    xTaskCreate( vTask_Pot1, "Pot1 Task", 2*configMINIMAL_STACK_SIZE, NULL, 2, &xTask_Pot1);
    xTaskCreate( vTask_Pot2, "Pot2 Task", 2*configMINIMAL_STACK_SIZE, NULL, 2, &xTask_Pot2);
    xTaskCreate( vTask_BubbleLevel, "Bubble Level Task", 2*configMINIMAL_STACK_SIZE, NULL, 2, &xTask_Bubble );
    xTaskCreate( vTask_KillBitGame, "KillBitGame", 2*configMINIMAL_STACK_SIZE, NULL, 8, &xTask_KillBitGame );
    vTaskSuspend(xTask_KillBitGame);
    /* Start the created tasks running. */
    xTimerStart(SensorTimer, 0);
    vTaskStartScheduler();

    /* Execution will only reach here if there was insufficient heap to
    start the scheduler. */
    for( ;; );
    return 0;
}



