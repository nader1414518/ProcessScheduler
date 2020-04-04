
#include "headers.h"
#include "queue.h"
#include "priority_queue.h"
#include <string.h>


pid_t pid;
key_t msgqid1;
//key_t msgqid2;
FILE * logFilePtr;
bool available = true;
pData runningProcess;
pData currentProc;
int WaitTime;

void hpfHandler(int signum);
void clearResources(int signum);

void  HPF() {
    //initializing some staff
    //printf("HPF Scheduler Initialized\n");
    bool firstIteration = 1;
    priority_heap_t *Q = (priority_heap_t*)calloc(1,sizeof(priority_heap_t));
    //printf("Priority Queue created\n");

    //Beginning of the Sceduler logic
    while(1) {
        //initializing the priority Q parameters
        if(firstIteration) {
            initializePQueue(Q);
            printf("Priority Q initialized\n");
            firstIteration = 0;
        }

        //Creating a container for the recieved process
        struct Data recievedProcess;
        pData recievedData;
        //Recieving Processes that are in the Message Queue
        while(1) {
            size_t rcv = msgrcv(msgqid1, &recievedProcess, sizeof(recievedProcess), 0, IPC_NOWAIT);
            if (rcv == -1) {
                //perror("Error in recieve\n");
                break;
            }
            else {
                printf("Message Recieved\n");
                recievedData.arrival = recievedProcess.arrival;
                recievedData.id = recievedProcess.id;
                recievedData.priority = recievedProcess.priority;
                recievedData.runtime = recievedProcess.runtime;
                //Forking  processes corresponding to the new Data recived
                int cpid = fork();
                if(cpid == -1) {
                    printf("Error in fork..\n");
                }
                else if(cpid == 0) {
                    //Passing the runtime to the child proccess
                    char buf[3];
                    sprintf(buf, "%d", recievedData.runtime);
                    char *argv[] = { "./process.out", buf, 0};
                    //printf("process %d forked\n", getpid());
                    execve(argv[0], &argv[0], NULL);
                }
                else {
                    //Adding the new processes received to the ready Queue
                    recievedData.id = cpid;
                    recievedData.isRunning = false;
                    push(Q, recievedData);
                    kill(cpid, SIGSTOP);
                    printf("Process %d is in ready state and forked at time %d\n", cpid, getClk());
                }
            }
        }
        //If the processor is not busy and there is ready processes set the appropriate process to active
        if (Q->len != 0 && available == true) {
            runningProcess = pop(Q);
            runningProcess.isRunning = 1;
            kill(runningProcess.id, SIGCONT);
            printf("Process %d is now active at time %d\n", runningProcess.id, getClk());
            WaitTime = getClk() - runningProcess.arrival;
            fprintf(logFilePtr, "At time %d process %d started arr %d total %dx remain %d wait %d\n",
                    getClk(),runningProcess.id, runningProcess.arrival, 0, runningProcess.runtime, WaitTime);

            available = false;
        }

        sleep(1);

    }
    //upon termination release the clock resources
    destroyClk(true);
}

void SRTN() {
    
    printf("intered SRTN algorithm\n" );
    bool firstIteration = 1;
    int processInterTime;
    priority_heap_t *Q = (priority_heap_t*)calloc(1,sizeof(priority_heap_t));
    printf("Priority Queu created\n");
    while(1){
        if(firstIteration){
            initializePQueue(Q);
            printf("Priority Q initialized\n");
            pid = getpid();
            firstIteration = 0;
        }
        struct Data recievedProcess;
        pData recievedData;
        //Recieving Processes that are in the Message Queue
        while(1) {
            size_t rcv = msgrcv(msgqid1, &recievedProcess, sizeof(recievedProcess), 0, IPC_NOWAIT);
            if (rcv == -1) {
                //perror("Error in recieve\n");
                break;
            }
            else {
                printf("Message Recieved\n");
                //count++;
                recievedData.arrival = recievedProcess.arrival;
                recievedData.id = recievedProcess.id;
                recievedData.priority = recievedProcess.priority;
                recievedData.runtime = recievedProcess.runtime;
            //fork a process for the comming data
                printf("\n %d %d\n",recievedData.arrival,recievedData.runtime );
                int cpid = fork();
                if(cpid == -1) {
                    printf("Error in fork..\n");
                }
                else if(cpid == 0) {
                char buf[3];
                // itoa(currentProcessData.runtime, buf, 10);
                sprintf(buf, "%d", recievedData.runtime);
                char *argv[] = { "./process.out", buf, 0};
                execve(argv[0], &argv[0], NULL);
                }
                else {
                    recievedData.id = cpid;
                    recievedData.isRunning = false;
                     //push(Q, recievedData);
                    printf("Process added %d to Q\n", cpid);
                    kill(cpid, SIGSTOP);
                    printf("Process %d is in ready state\n", cpid);

                    if (available == true)
                    {
                        printf("there was no processes running so this process started\n" );
                        runningProcess.id = recievedData.id;
                        runningProcess.runtime = recievedData.runtime;
                        //  printf("\n data id %d\n",cpid );
                        runningProcess.isRunning = 1;
                        available = false;
                        processInterTime = getClk();
			runningProcess.remainingT = runningProcess.runtime;
                        //WaitTime = (runningProcess.arrival-getClk());
			//wait time here should = 0 because there was no processes running so it worked directly
			runningProcess.arrival =processInterTime;
			WaitTime = 0;
                        printf("Process %d is now active\n", runningProcess.id);
                        fprintf(logFilePtr, "At time %d process %d started arr %d total %d remain %d wait %d\n",
                                getClk(),runningProcess.id, runningProcess.arrival,0, runningProcess.runtime, WaitTime );
                        printf("process inter time %d\n",processInterTime);
                        kill(runningProcess.id,SIGCONT);
                    }
                    else {
                        //calculate the time passed in the currentProc
                        runningProcess.remainingT = runningProcess.runtime - (getClk() - processInterTime);
                        //if it is there one running compare between the running time
                        printf("runningProcess.remainingT %d\n",runningProcess.remainingT );
			printf("recievedData.runtime %d \n",recievedData.runtime);
                        if(recievedData.runtime < runningProcess.remainingT )
                        {
                            kill(runningProcess.id,SIGSTOP);
				WaitTime = (getClk() - runningProcess.arrival - runningProcess.runtime + runningProcess.remainingT);
 				fprintf(logFilePtr, "At time %d process %d paused arr %d total %d remain %d wait %d\n",
                                getClk(),runningProcess.id, runningProcess.arrival,0, runningProcess.remainingT, WaitTime );
                            runningProcess.priority = runningProcess.remainingT;
                            push(Q,runningProcess);
                            runningProcess.id = recievedData.id;
                            runningProcess.arrival = recievedData.arrival;
                            runningProcess.priority = recievedData.priority;
                            runningProcess.remainingT = recievedData.runtime;
                            runningProcess.runtime = recievedData.runtime;
                            runningProcess.isRunning = 1;
                            processInterTime = getClk();
				WaitTime = (getClk() - runningProcess.arrival - runningProcess.runtime + runningProcess.remainingT);
 				fprintf(logFilePtr, "At time %d process %d resumed arr %d total %d remain %d wait %d\n",
                                getClk(),runningProcess.id, runningProcess.arrival,0, runningProcess.remainingT, WaitTime );
                            kill(recievedData.id,SIGCONT);
                        }
                        else
                        {
                            recievedData.priority = recievedData.runtime;
                            push(Q,recievedData);
                        }
                    }
                } 
            }
        } 
        if (Q->len != 0 && available == true) {
                printf("\nCondition is true\n");
                runningProcess = pop(Q);
                runningProcess.isRunning = 1;
                WaitTime = (getClk() - runningProcess.arrival);
                printf("Process %d is now active\n", runningProcess.id);
                fprintf(logFilePtr, "At time %d process %d started arr %d total %d remain %d wait %d\n",
                    getClk(),runningProcess.id, runningProcess.arrival,0, runningProcess.runtime, WaitTime );
                kill(runningProcess.id, SIGCONT);
                available = false;
        }  
    }
    /*fprintf(PerfFilePtr, "CPU utilization = 100\% \n");
    fprintf(PerfFilePtr, "Avg WTA = %d \n",WTA/count);
    fprintf(PerfFilePtr, "Avg Wait = %d \n",TotalTA/count); */
    destroyClk(true);
}    


pData recv_msg_process(key_t msgqid)
{
    int recv_val;
    pData processData;

    // receive all messages
    recv_val = msgrcv(msgqid, &processData, sizeof(processData), 0, !IPC_NOWAIT);

    if (recv_val == -1)
    {
        perror("Error in receive, bad data returned");
        return processData;
    }
    else
    {
        printf("\nProcess Data received\n");
        return processData;
    }
}




void main(int argc, char * argv[])
{
    signal(SIGUSR1, hpfHandler);
    signal(SIGINT,clearResources);

    initClk();
    msgqid1 = msgget(16499, 0644);

    logFilePtr = fopen("scheduler.log", "w");
    if (logFilePtr == NULL)
    {
        printf("Unable to create log file for the scheduler ... \n");
    }


    if (strcmp(argv[1], "-HPF") == 0) {
        printf("HPF Scheduler Initialized\n");
        HPF();
    }
    else if (strcmp(argv[1], "-SRTN") == 0) {
        printf("SRTN Scheduler Initialized\n");
        SRTN();
    }
    else if (strcmp(argv[1], "-RR") == 0){
        printf("RR Scheduler Initialized\n");

        printf("RR Scheduler Initialized\n");
        char *s_argv[] = {"./rr_scheduler.out","-RR", 0};
	    execve(s_argv[0], &s_argv[0], NULL);
    }

    // scheduler logic


}
void hpfHandler(int signum) {
    
    printf("Process %d has finished at time %d\n", runningProcess.id, getClk());
    WaitTime = (getClk() - runningProcess.arrival - runningProcess.runtime);
    float WTA = ((float)(getClk() - runningProcess.arrival))/((float)(runningProcess.runtime));
    fprintf(logFilePtr, "At time %d process %d finished arr %d total %d remain %d wait %d TA %d WTA %0.2f\n",
            getClk(),runningProcess.id, runningProcess.arrival, runningProcess.runtime, 0, WaitTime, (getClk() - runningProcess.arrival) /*TA*/, WTA /*WTA*/);

    available = true;
    signal(SIGUSR1, hpfHandler);
}
void clearResources(int signum) {
    msgctl(msgqid1, IPC_RMID, (struct msqid_ds *) 0);
    // msgctl(msgqid2, IPC_RMID, (struct msqid_ds *) 0);
    printf(" Interruption detected ... ");
    exit(0);
}
