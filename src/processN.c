#include "headers.h"
#include <time.h>

void pause_handler(int signum);
void continue_handler(int signum);
void alarm_handler(int signum);

/* Modify this file as needed*/
int remainingtime;
int runtime;

void main(int argc, char * argv[])
{
    // initClk();
    signal(SIGTSTP, pause_handler);
    signal(SIGCONT, continue_handler);
    signal(SIGALRM, alarm_handler);
    
    // the process should run for specific runtime (may be passed as 
    // an argument) and the remaining time will be equal to 
    // (runtime - (int)clock()/CLOCKS_PER_SEC)
    
    // get the runtime argument
    runtime = (int)atoi(argv[1]);
    printf("Runtime: %d\n", runtime);
    
    //TODO it needs to get the remaining time from somewhere
	remainingtime = runtime - (int)clock()/CLOCKS_PER_SEC;	/* number of processor ticks since
	the begining of the process */
	printf("Remaining Time: %d\n", remainingtime);
	while (remainingtime > 0)
	{
	 	remainingtime = runtime - (int)clock()/CLOCKS_PER_SEC;
		// printf("Elapsed time: %d\n", remainingtime);
	}
    
    // raise(SIGSTOP);
    exit(0);
    // destroyClk(false);
    // exit(0);
}

void pause_handler(int signum)
{
	printf("Process Paused ... ");
	exit(1);	// 1: paused, 0: finished
}

void continue_handler(int signum)
{
	printf("Process Continued ... ");
}

void alarm_handler(int signum)
{
	// pause the process
	raise(SIGTSTP);
}
