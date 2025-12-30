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


extern char* my_fgets(char *, int, FILE *);

/*-------------------------------------------------------------------------+
| Headers of command functions
+--------------------------------------------------------------------------*/ 
void cmd_help  (int, char** );
void cmd_test  (int, char** );
extern void cmd_readdatetime  (int, char** );
extern void cmd_readclock  (int, char** );
extern void cmd_setdate  (int, char** );
extern void cmd_setclock  (int, char** );
extern void cmd_readtemp  (int, char** );
extern void cmd_readminmax  (int, char** );
extern void cmd_clearminmax  (int, char** );
extern void cmd_readparams  (int, char** );
extern void cmd_modmonperiod  (int, char** );
extern void cmd_modtimealarm  (int, char** );
extern void cmd_readalarminfo  (int, char** );
extern void cmd_setalarmclock  (int, char** );
extern void cmd_setalarmtemp  (int, char** );
extern void cmd_alarmclocken  (int, char** );
extern void cmd_tempalarmen  (int, char** );
extern void cmd_readtaskstate  (int, char** );
extern void cmd_bubblelevelen  (int, char** );
extern void cmd_hitbiten  (int, char** );
extern void cmd_configsounden  (int, char** );

/*-------------------------------------------------------------------------+
| Variable and constants definition
+--------------------------------------------------------------------------*/ 
const char TitleMsg[] = "\n Application Control Monitor\n";

struct  command_d {
  void  (*cmd_fnct)(int, char**);
  char* cmd_name;
  char* cmd_help;
  int arguments;
} const commands[] = {
  {cmd_help,         "help","                : show help", 0},
  {cmd_readdatetime, "rdt","                 : read date/time", 0},
  {cmd_readclock,    "rc","                  : read time", 0},
  {cmd_setdate,      "sd"," <dd> <MM> <YYYY> : set date", 3},
  {cmd_setclock,     "sc"," <hh> <mm> <ss>   : set clock time", 3},
  {cmd_readtemp,     "rt","                  : read temperature [Celsius]", 0},
  {cmd_readminmax,   "rmm","                 : read min/max temp records [Celsius]", 0},
  {cmd_clearminmax,  "cmm","                 : clear min/max temp records [Celsius]", 0},
  {cmd_readparams,   "rp","                  : read parameters: PMON, TALA", 0},
  {cmd_modmonperiod, "mmp"," <PMON>          : set monitoring period [s] (0 to disable)", 1},
  {cmd_modtimealarm, "mta"," <TALA>          : set alarm duration [s]", 1},
  {cmd_readalarminfo,"rai","                 : read alarm settings", 0},
  {cmd_setalarmclock,"sac"," <hh> <mm> <ss>  : set alarm clock time", 3},
  {cmd_setalarmtemp, "sat"," <lowT> <highT>  : set temperature limits", 2},
  {cmd_alarmclocken, "adac"," <1/0>          : enable/disable alarm clock", 1},
  {cmd_tempalarmen,  "adat"," <1/0>          : enable/disable temperature alarm", 1},
  {cmd_readtaskstate,"rts","                 : read task status (on/off)", 0},
  {cmd_bubblelevelen,"adbl"," <1/0>          : enable/disable bubble level", 1},
  {cmd_hitbiten,     "adhb"," <1/0>          : enable/disable hit bit game", 1},
  {cmd_configsounden,"adcs"," <1/0>          : enable/disable config sound", 1},
  {cmd_test,         "test"," <a1> <a2 <a3>  : TESTING - prints out argv", 3 }
};

#define NCOMMANDS  (sizeof(commands)/sizeof(struct command_d))
#define ARGVECSIZE 4
// must be 4 - 1 command and 3 arguments
#define MAX_LINE   50


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

    printf("%s Type \"help\" for help\n", TitleMsg);
    for (;;) {
        printf("\ncmd> ");
        /* Reading and parsing command line  ----------------------------------*/
        if ((argc = my_getline(argv, ARGVECSIZE)) > 0) {
            for (p=argv[0]; *p != '\0'; *p=tolower(*p), p++);
            for (i = 0; i < NCOMMANDS; i++) 
                if (strcmp(argv[0], commands[i].cmd_name) == 0) 
                    break;
        /* Executing commands -----------------------------------------------*/
            if (i < NCOMMANDS) {
                //printf("argc: %d\n", argc);
                if ( (argc - 1) == commands[i].arguments )
                    commands[i].cmd_fnct (argc, argv);
                else
                    printf("Wrong number of arguments");
            }
            else
                printf("Invalid command");
        } /* if my_getline */
  } /* forever */
}

/*-------------------------------------------------------------------------+
| Function: cmd_help - provides a rudimentary help
+--------------------------------------------------------------------------*/ 
void cmd_help (int argc, char **argv)
{

  printf("%s", TitleMsg);
  printf(" Format:\n");
  printf(" command <arguments> : explanation\n");
  printf("----------------------------------\n");
  for (int i=0; i<NCOMMANDS; i++)
    printf(" %s%s\n", commands[i].cmd_name, commands[i].cmd_help);
}

/*-------------------------------------------------------------------------+
| Function: cmd_test - print out arguments
+--------------------------------------------------------------------------*/
void cmd_test(int argc, char **argv) {
    int i;

    for (i = 0; i < argc; i++)
        printf("argv[%d] = %s\n", i, argv[i]);
}

//#endif //notdef
