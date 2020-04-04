#include "headers.h"

/* Modify this file as needed*/
int remainingtime;

int main(int agrc, char * argv[])
{
    initClk();
    int remainingtime = atoi(argv[1]);
    printf("process runtime is %d\n", remainingtime);

    //TODO it needs to get the remaining time from somewhere
    //remainingtime = ??;
    while (remainingtime > 0)
    {
        sleep(1);
        remainingtime--;
        // remainingtime = ??;
    }
    kill(getppid(), SIGUSR1);
    destroyClk(false);

    return 0;
}
