//#ifdef notdef

/***************************************************************************
| File: monitor.c
|
| Autor: Carlos Almeida (IST), from work by Jose Rufino (IST/INESC), 
|        from an original by Leendert Van Doorn
| Data:  Nov 2002
***************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
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

extern char* my_fgets(char *, int, FILE *);
extern SemaphoreHandle_t ClockMutex;
extern SemaphoreHandle_t RecordMutex;
extern SemaphoreHandle_t ParamMutex;

extern Record maxtemp;
extern Record mintemp;
extern volatile int TL;
extern volatile int TM;
extern volatile int PMON;
extern volatile int TALA;
extern volatile bool alarm;
extern TaskHandle_t xTask_temp;
extern void alarmFunction(void);
tm alarm_time = RTC::getDefaultTM();
/*-------------------------------------------------------------------------+
| Headers of command functions
+--------------------------------------------------------------------------*/ 
extern void cmd_sair (int, char** );
extern void cmd_test (int, char** );
       void cmd_sos  (int, char** );
extern void cmd_send (int, char** );
extern void cmd_rdt (int,char**);
extern void cmd_sd (int,char**);
extern void cmd_rc (int,char**);
extern void cmd_rd (int,char**);
extern void cmd_sc (int,char**);
extern void cmd_rmm (int,char**);
extern void cmd_cmm (int,char**);
extern void cmd_rt (int,char**);
extern void cmd_rp (int,char**);
extern void cmd_mmp (int,char**);
extern void cmd_mta (int,char**);
extern void cmd_rai (int,char**);
extern void cmd_sac (int,char**);
extern void cmd_sat (int,char**);

/*-------------------------------------------------------------------------+
| Variable and constants definition
+--------------------------------------------------------------------------*/ 
const char TitleMsg[] = "\n Application Control Monitor\n";
const char InvalMsg[] = "\nInvalid command!";

struct  command_d {
  void  (*cmd_fnct)(int, char**);
  char* cmd_name;
  char* cmd_help;
} const commands[] = {
  {cmd_sos,  "sos","                  help"},
  {cmd_send, "send","<msg>            send message"},
  {cmd_sair, "sair","                 sair"},
  {cmd_test, "test","<arg1> <arg2>    test command_2"},
  {cmd_rdt, "rdt" ,"                  reads date and time"},          
  {cmd_sd,"sd","<dd> <MM> <YY>        sets date(dd:MM:YY)"},
  {cmd_rd,"rd","                      reads time"},
  {cmd_sc,"sc","<hh> <mm> <ss>        sets time(hh:mm:ss)"},
  {cmd_rt,"rt","                      reads temperature"},
  {cmd_rmm,"rmm","                    reads max and min temperature"},
  {cmd_cmm,"cmm","                    clears max and min temperature"},
  {cmd_rp,"rp","                      reads parameters (PMON,TALA)"},
  {cmd_mmp,"mpp","<p>                 modify PMON (PMON,TALA)"},
  {cmd_mta,"mta","<s>                 modify TAla (PMON,TALA)"},
  {cmd_rai,"rai","<s>                 read alarm info (alarm clock, tlow, thigh, active/inactive)"},
  {cmd_sac,"sac","<hh> <mm> <ss>      set alarm clock (hh:mm:ss)"},
  {cmd_sat,"sat","<tl> <tm>           set alarm threshold (tlow,thigh)"},



};

#define NCOMMANDS  (sizeof(commands)/sizeof(struct command_d))
#define ARGVECSIZE 3
#define MAX_LINE   50

/*-------------------------------------------------------------------------+
| Function: cmd_sos - provides a rudimentary help
+--------------------------------------------------------------------------*/ 
void cmd_sos (int argc, char **argv)
{
  int i;

  printf("%s\n", TitleMsg);
  for (i=0; i<NCOMMANDS; i++)
    printf("%s %s\n", commands[i].cmd_name, commands[i].cmd_help);
}

/*-------------------------------------------------------------------------+
| Function: my_getline        (called from monitor) 
+--------------------------------------------------------------------------*/ 
int my_getline (char** argv, int argvsize)
{
  static char line[MAX_LINE];
  char *p;
  int argc;

//  fgets(line, MAX_LINE, stdin);
  my_fgets(line, MAX_LINE, stdin);

  /* Break command line into an o.s. like argument vector,
     i.e. compliant with the (int argc, char **argv) specification --------*/

  for (argc=0,p=line; (*line != '\0') && (argc < argvsize); p=NULL,argc++) {
    p = strtok(p, " \t\n");
    argv[argc] = p;
    if (p == NULL) return argc;
  }
  argv[argc] = p;
  return argc;
}

/*-------------------------------------------------------------------------+
| Function: monitor        (called from main) 
+--------------------------------------------------------------------------*/ 
void monitor (void)
{
  static char *argv[ARGVECSIZE+1], *p;
  int argc, i;

  printf("%s Type sos for help\n", TitleMsg);
  for (;;) {
    printf("\nCmd> ");
    /* Reading and parsing command line  ----------------------------------*/
    if ((argc = my_getline(argv, ARGVECSIZE)) > 0) {
      for (p=argv[0]; *p != '\0'; *p=tolower(*p), p++);
      for (i = 0; i < NCOMMANDS; i++) 
    if (strcmp(argv[0], commands[i].cmd_name) == 0) 
      break;
      /* Executing commands -----------------------------------------------*/
      if (i < NCOMMANDS)
    commands[i].cmd_fnct (argc, argv);
      else  
    printf("%s", InvalMsg);
    } /* if my_getline */
  } /* forever */
}

void cmd_rdt(int argc, char** argv){
    if(xSemaphoreTake(ClockMutex, 100)) {

        tm tm;
        time_t t;
        time(&t);
        localtime_r(&t, &tm);
        printf("%s\n", ctime(&t));
        xSemaphoreGive(ClockMutex);
    } else {
        printf("Failed to acquire ClockMutex\n");
    }
}   
void cmd_sd(int argc, char** argv){

    if(xSemaphoreTake(ClockMutex, 100)) {
        struct tm t;
        time_t seconds;

        time(&seconds);
        localtime_r(&seconds, &t);

        t.tm_mday = atoi(argv[1]);
        t.tm_mon  = atoi(argv[2]) - 1;
        t.tm_year = atoi(argv[3]) - 1900;

        seconds = mktime(&t);
        set_time(seconds);

        xSemaphoreGive(ClockMutex);
    } else {
        printf("Failed to acquire ClockMutex\n");
    }
}

void cmd_rc(int argc,char**argcv){
    tm tm;
    time_t t;
    if(xSemaphoreTake(ClockMutex, 100)) {
        time(&t);
        localtime_r(&t, &tm);
        printf("%d:%d:%d\n",tm.tm_hour,tm.tm_min,tm.tm_sec);
        xSemaphoreGive(ClockMutex);
    } else {
        printf("Failed to acquire ClockMutex\n");
    }

}
void cmd_rd(int argc,char**argcv){
    tm tm;
    time_t t;
    if(xSemaphoreTake(ClockMutex, 100)) {
        time(&t);
        localtime_r(&t, &tm);
        printf("%d/%d/%d\n",tm.tm_mday,tm.tm_mon,tm.tm_year);
        xSemaphoreGive(ClockMutex);
    } else {
        printf("Failed to acquire ClockMutex\n");
    }

}
void cmd_sc(int argc, char** argv){
    struct tm t;
    time_t seconds;
    if(xSemaphoreTake(ClockMutex, 100)) {

        time(&seconds);
        localtime_r(&seconds, &t);

        t.tm_hour = atoi(argv[1]);
        t.tm_min  = atoi(argv[2]) - 1;
        t.tm_sec = atoi(argv[3]) - 1900;

        seconds = mktime(&t);
        set_time(seconds);
        xSemaphoreGive(ClockMutex);
    } else {
        printf("Failed to acquire ClockMutex\n");
    }
}
void cmd_rmm(int argc, char** argv){
    if(xSemaphoreTake(RecordMutex, 100)){
        printf("Max Temp: %d C  @ %02d:%02d:%02d\n",
               maxtemp.temp,
               maxtemp.timestamp.tm_hour,
               maxtemp.timestamp.tm_min,
               maxtemp.timestamp.tm_sec);

        printf("Min Temp: %d C  @ %02d:%02d:%02d\n",
               mintemp.temp,
               mintemp.timestamp.tm_hour,
               mintemp.timestamp.tm_min,
               mintemp.timestamp.tm_sec);

        xSemaphoreGive(RecordMutex);
    } else {
        printf("Failed to acquire RecordMutex\n");
    }
}

void cmd_cmm(int argc, char** argv){
    if(xSemaphoreTake(RecordMutex, 100)){
        maxtemp.temp = -1;
        maxtemp.timestamp.tm_hour = -1;
        maxtemp.timestamp.tm_min = -1;
        maxtemp.timestamp.tm_sec = -1;
        mintemp.temp = -1;
        mintemp.timestamp.tm_hour = -1;
        mintemp.timestamp.tm_min = -1;
        mintemp.timestamp.tm_sec = -1;
        xSemaphoreGive(RecordMutex);
    } else {
        printf("Failed to acquire RecordMutex\n");
    }
}
void cmd_rt(int argc, char** argv){
    xTaskNotify(xTask_temp, 0,eNoAction);
    }
void cmd_rp(int argc, char** argv){
    if(xSemaphoreTake(ParamMutex, 100)){
        printf("PMON %d     TALA %d",PMON,TALA);
        xSemaphoreGive(ParamMutex);
    } else {
        printf("Failed to acquire RecordMutex\n");
    }
}
void cmd_mmp(int argc, char** argv){
    if(xSemaphoreTake(ParamMutex, 100))
        {
        PMON = atoi(argv[0]);
        xSemaphoreGive(ParamMutex);
        }
    if(PMON > 0){xTaskNotify(xTask_temp, 0,eNoAction);}
    }
void cmd_mta(int argc, char** argv){
    if(xSemaphoreTake(ParamMutex, 100))
        TALA = atoi(argv[0]);
        xSemaphoreGive(ParamMutex);
    }

void cmd_rai(int argc, char** argv){
    if(xSemaphoreTake(ParamMutex, 200)){
        printf(
        "Alarm time: %02d:%02d:%02d\n"
        "Min temperature: %d C\n"
        "Max temperature: %d C\n"
        "Alarm active: %s\n",
        alarm_time.tm_hour,
        alarm_time.tm_min,
        alarm_time.tm_sec,
        TL,
        TM,
        alarm ? "ON" : "OFF");

        xSemaphoreGive(ParamMutex);
    }
    
}

void cmd_sac(int argc, char** argv){
    if((atoi(argv[0]) <= 23 && atoi(argv[0]) >= 0)
     ||(atoi(argv[1]) <= 59 && atoi(argv[1]) >= 0)
     ||(atoi(argv[2]) <= 59 && atoi(argv[2]) >= 0))
     {
        alarm_time.tm_hour = atoi(argv[0]);
        alarm_time.tm_min = atoi(argv[1]);
        alarm_time.tm_min = atoi(argv[2]);

        RTC::alarm(&alarmFunction, alarm_time);
     }
}
void cmd_sat(int argc, char** argv){
    if((atoi(argv[0]) <= 50 && atoi(argv[0]) >= 0)
     ||(atoi(argv[1]) <= 50 && atoi(argv[1]) >= 0))
     {
        if(xSemaphoreTake(ParamMutex, 200)){
        
            TL = atoi(argv[0]);
            TM = atoi(argv[1]);
            xSemaphoreGive(ParamMutex);
        }
     }
}
//#endif //notdef
