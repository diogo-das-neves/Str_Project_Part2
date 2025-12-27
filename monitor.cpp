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

extern char* my_fgets(char *, int, FILE *);
extern SemaphoreHandle_t ClockMutex;
extern Record maxtemp;
extern Record mintemp;
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
  {cmd_rdt, "rdt" ,"                  read date and time"},          
  {cmd_sd,"sd","<dd> <MM> <YY>        set date(dd:MM:YY)"},
  {cmd_rd,"sd","                      read time"}
  {cmd_sc,"sd","<hh> <mm> <ss>        set time(hh:mm:ss)"},


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
    struct tm t;
    time_t seconds;

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
//#endif //notdef
