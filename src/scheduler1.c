
#include "headers.h"
#include "queue.h"
#include "priority_queue.h"
#include <string.h>


pid_t pid;
key_t msgqid1;
//key_t msgqid2;
FILE * logFilePtr;
FILE * PerfFilePtr;
bool available = true;
pData runningProcess;
pData currentProc;
int WaitTime;
int WTA;
int TotalTA;
int count = 0;
void hpfHandler(int signum);
void clearResources(int signum);

void  HPF() {
    //initializing some staff
    printf("HPF Scheduler Initialized\n");
    bool firstIteration = 1;
    priority_heap_t *Q = (priority_heap_t*)calloc(1,sizeof(priority_heap_t));
    printf("Priority Queu created\n");

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
                count++;
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
                    printf("process %d forked\n", getpid());
                    execve(argv[0], &argv[0], NULL);
                }
                else {
                    //Adding the new processes received to the ready Queue
                    recievedData.id = cpid;
                    recievedData.isRunning = false;
                    push(Q, recievedData);
                    kill(cpid, SIGSTOP);
                    printf("Process %d is in ready state\n", cpid);
                }
            }
        }
        //If the processor is not busy and there is ready processes set the appropriate process to active
        if (Q->len != 0 && available == true) {
            runningProcess = pop(Q);
            runningProcess.isRunning = 1;
            printf("Process %d is now active\n", runningProcess.id);
            
         fprintf(logFilePtr, "At time %d process %d started arr %d remain %d wait %d\n",
                    getClk(),runningProcess.id, runningProcess.arrival, runningProcess.runtime, (runningProcess.arrival-getClk()));
            kill(runningProcess.id, SIGCONT);
            available = false;
        }

        sleep(1);

    }
    //upon termination release the clock resources
    fprintf(PerfFilePtr, "CPU utilization = 100\% \n");
    fprintf(PerfFilePtr, "Avg WTA = %d \n",WTA/count);
    fprintf(PerfFilePtr, "Avg Wait = %d \n",TotalTA/count); 
    destroyClk(true);
}

void SRTN(){
    printf("intered SRTN algorithm\n" );
    bool firstIteration = 1;
    long processInterTime;
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
                count++;
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
                        printf("process inter time %d\n",processInterTime);
                        kill(runningProcess.id,SIGCONT);
                    }
                    else {
                        //if there is no process running run the current one
                        //calculate the time passed in the currentProc
                        runningProcess.remainingT = runningProcess.runtime - (getClk() - processInterTime);
                        //if it is there one running compare between the running time
                        printf("runningProcess.remainingT %d\n",runningProcess.remainingT );
                        if(recievedData.remainingT < runningProcess.remainingT )
                        {
                            kill(runningProcess.id,SIGSTOP);

                            //kill(recievedData.id,SIGCONT); //it is already running
                            //reaProc = insertSorted(reaProc,currentProc);
                            runningProcess.priority = runningProcess.remainingT;
                            push(Q,runningProcess);
                            // it will be more effiecient if you write copy constructor
                            runningProcess.id = recievedData.id;
                            runningProcess.arrival = recievedData.arrival;
                            runningProcess.priority = recievedData.priority;
                            runningProcess.remainingT = recievedData.remainingT;
                            runningProcess.runtime = recievedData.runtime;
                            processInterTime = getClk();
                            kill(recievedData.id,SIGCONT);
                        }
                        else
                        {
                    // use insert sorted to insert the one you want in the ready queue
                    //reaProc = insertSorted(reaProc,recievedData);
                            recievedData.priority = recievedData.runtime;
                            push(Q,recievedData);
                        }
                    }
                } 
            }
        } 
        if (Q->len != 0 && available == true) {
                printf("Condition is true\n");
                runningProcess = pop(Q);
                runningProcess.isRunning = 1;
                printf("Process %d is now active\n", runningProcess.id);
                fprintf(logFilePtr, "At time %d process %d started arr %d remain %d wait %d\n",
                    getClk(),runningProcess.id, runningProcess.arrival, runningProcess.runtime, (runningProcess.arrival-getClk()));
                kill(runningProcess.id, SIGCONT);
                available = false;
        }  
    }
    fprintf(PerfFilePtr, "CPU utilization = 100\% \n");
    fprintf(PerfFilePtr, "Avg WTA = %d \n",WTA/count);
    fprintf(PerfFilePtr, "Avg Wait = %d \n",TotalTA/count); 
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
    TotalTA = 0;
    PerfFilePtr = fopen("scheduler.perf", "w");
    if (PerfFilePtr == NULL)
    {
        printf("Unable to create log file for the scheduler ... \n");
    }

    if (strcmp(argv[1], "-HPF") == 0){
        printf("HPF Scheduler Initialized\n");
        HPF();
    }
    else if (strcmp(argv[1], "-SRTN") == 0){
        printf("SRTN Scheduler Initialized\n");
        SRTN();
    }
    else if (strcmp(argv[1], "-RR") == 0){
        printf("RR Scheduler Initialized\n");
        char *s_argv[] = {"./rr_scheduler.out","-RR", 0};
	    execve(s_argv[0], &s_argv[0], NULL);

    }
        
    


    // scheduler logic
    
    
}
void hpfHandler(int signum){
    printf("Process %d has finished at time %d\n", runningProcess.id, getClk());
    WaitTime = getClk() - runningProcess.arrival;
    TotalTA = TotalTA + WaitTime;
    WTA = WTA + (int)((getClk() - runningProcess.arrival)/ runningProcess.runtime);
    fprintf(logFilePtr, "At time %d process %d finished arr %d total %d remain %d wait %d TA %d WTA %0.2f\n",
                                getClk(),runningProcess.id, runningProcess.arrival, runningProcess.runtime, 0, WaitTime, (getClk() - runningProcess.arrival), (float)((getClk() - runningProcess.arrival)/ runningProcess.runtime));
    available = true;
    signal(SIGCHLD, hpfHandler);
}
void clearResources(int signum){
    msgctl(msgqid1, IPC_RMID, (struct msqid_ds *) 0);
   // msgctl(msgqid2, IPC_RMID, (struct msqid_ds *) 0);
    printf(" Interruption detected ... ");
    exit(0);
}
