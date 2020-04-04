
#include "headers.h"
#include "queue.h"
#include "priority_queue.h"
#include <string.h>
#include <sys/mman.h>	// for the purpose of shared memory
#include <fcntl.h>
#include <sys/stat.h>
#include <math.h>


pid_t pid;
key_t msgqid1;
//key_t msgqid2;
FILE * logFilePtr;
FILE * perfFilePtr;
bool available = true;
pData runningProcess;
pData currentProc;
int WaitTime;

void hpfHandler(int signum);
void clearResources(int signum);

void RR_Schedule(int * shared_quanta_memory, struct Queue * shared_processes_memory)
{
	// processes ready data
	// struct Queue readyQueue;
	// initializeQueue(readyQueue);
	// readyQueue.front = readyQueue.rear = 0;
	shared_processes_memory->rear = shared_processes_memory->front = 0;
	int q;
	// for CPU Utilization
	int total_time = 0;
	int use_time = 0;
	// int * turnaround_times;
	/* Uncomment this to receive the whole processes queue */
	// prc_queue = recv_msg(msgqid2);
	int waiting_interval = 0;
	printf("Q: ");
	scanf("%d", &q);
	printf("RR is selected with Q = %d\n", q);
	// receive process by process (fork a process to wait for messages)
	int serv_pid = fork();
	if (serv_pid == -1)
	{
		perror("Error in forking .. ");
	}
	else if (serv_pid == 0)
	{
		shared_quanta_memory[0] = q;
		initClk();
		printf("Receiving Messages from process generator .. \n");
		// processes received data
		// struct Queue tempQueue;
		// initializeQueue(tempQueue);
		int i = shared_processes_memory->front;
		while (1)
		{
			struct Data data;
			if (msgrcv(msgqid1, &data, sizeof(data), 0, !IPC_NOWAIT) == -1)
			{
				printf("Error in receiving messages ...\n");
			}
			else
			{
				shared_processes_memory->dataArray[shared_processes_memory->rear] = data;
				strcpy(shared_processes_memory->dataArray[shared_processes_memory->rear].status, "new");
				shared_processes_memory->dataArray[shared_processes_memory->rear].last_processed = getClk();
				shared_processes_memory->rear++;
				i++;
			}
			
			// sleep(1);
		}
	}
	else
	{
		// current running process
		struct Data currentProcessData;
		struct Queue tempDataQueue;
		tempDataQueue.front = tempDataQueue.rear = 0;
		
		while (1)
		{
		    // printf("Scheduler Running .. with Q = %d\n", shared_quanta_memory[0]);
		    // readyQueue.rear = shared_processes_memory->rear;
		    // printf("No. of processes in ready queue: %d\n", readyQueue.rear);
		    printf("No. of processes in the shared memory: %d\n", shared_processes_memory->rear);
		    // printf("Ready Processes Count: %d\n", readyQueue.rear);
		    // new data received then reset the timer
		    // fork a process then wait for its termination 
		    // (you should be able to detect when it will terminate
		    // (or pause)
		    // Forking a process
		    /* Check if it is new or waiting */
		    /* Add the frontier process to the processing queue */
		    if (shared_processes_memory->rear > 0)
		    {
		        for (int j = 0; j < shared_processes_memory->rear; j++)
		        {
		            if (strcmp(shared_processes_memory->dataArray[j].status, "new") == 0)
		            {
		                total_time += shared_processes_memory->dataArray[j].runtime;
		                strcpy(shared_processes_memory->dataArray[j].status, "queued");
		            }
		        }
		    
		        currentProcessData = shared_processes_memory->dataArray[shared_processes_memory->front];
		        strcpy(currentProcessData.status, "running");
		        
		        waiting_interval = 0;
		        for (int i = 0; i < shared_processes_memory->rear-1; i++)
		        {
			        shared_processes_memory->dataArray[i] = shared_processes_memory->dataArray[i+1];
			        // tempDataQueue.dataArray[i]= tempDataQueue.dataArray[i+1];
		        }
		        shared_processes_memory->rear--;
		        // tempDataQueue.rear--;
		        // printf("Ready Queue Frontier (ID): %d\n", shared_processes_memory->dataArray[shared_processes_memory->front].id);
		        /* fork the process */
		        int prc_pid = fork();
		        if (prc_pid == -1)
		        {
			        perror("Could not start process ... \n");
		        }
		        else if (prc_pid == 0)
		        {
			        printf("Process %d started \n", currentProcessData.id);
			        fprintf(logFilePtr, "At time %d process %d started arr %d remain %d wait %d\n", getClk(), currentProcessData.id, currentProcessData.arrival, shared_processes_memory->dataArray[currentProcessData.id-1].runtime, getClk() - currentProcessData.last_processed);
			        // currentProcessData.status = "running";
			        alarm(shared_quanta_memory[0]);
			        char buf[3];
			        // itoa(currentProcessData.runtime, buf, 10);
			        // printf("Passed Runtime: %d\n", currentProcessData.runtime);
			        sprintf(buf, "%d", currentProcessData.runtime);
			        char *prc_argv[] = {"./processN.out", buf, 0};
			        execve(prc_argv[0], &prc_argv[0], NULL);
		        }
		        else 
		        {
			        int prc_status;
			        int prc_ret = wait(&prc_status);
			        if (prc_ret >= 0) 
			        {
				        if (WEXITSTATUS(prc_status) == 1)
				        {
					        // process terminated with pause exit code
					        /* put it at the rear of the queue */
					        fprintf(logFilePtr, "At time %d process %d started arr %d remain %d wait %d\n", getClk()-shared_quanta_memory[0], currentProcessData.id, currentProcessData.arrival, currentProcessData.runtime, getClk()-currentProcessData.last_processed);
					        currentProcessData.runtime = currentProcessData.runtime - shared_quanta_memory[0];
					        // printf("Expected runtime for p%d: %d\n", currentProcessData.id, currentProcessData.runtime);
					        currentProcessData.last_processed = getClk();
					        shared_processes_memory->dataArray[shared_processes_memory->rear] = currentProcessData;
					        shared_processes_memory->rear++;
					        // output something here to the log file
					        printf("Process %d paused \n", currentProcessData.id);
					        strcpy(currentProcessData.status, "paused");
					        fprintf(logFilePtr, "At time %d process %d paused arr %d remain %d wait %d\n", getClk(), currentProcessData.id, currentProcessData.arrival, shared_processes_memory->dataArray[currentProcessData.id-1].runtime, getClk()-currentProcessData.last_processed);
				        }
				        if (WEXITSTATUS(prc_status) == 0)
				        {
					        // process terminated with finish exit code
					        /* drop it */
					        // output something here to the log file 
					        fprintf(logFilePtr, "At time %d process %d started arr %d remain %d wait %d\n", getClk()-currentProcessData.runtime, currentProcessData.id, currentProcessData.arrival, currentProcessData.runtime, getClk()-currentProcessData.last_processed);
					        printf("Process %d stopped\n", currentProcessData.id);
					        currentProcessData.last_processed = getClk();
					        strcpy(currentProcessData.status, "stopped");
					        fprintf(logFilePtr, "At time %d process %d stopped arr %d remain %d wait %d\n", getClk(), currentProcessData.id, currentProcessData.arrival, currentProcessData.runtime, getClk()-currentProcessData.last_processed);
				        }
			        }
		        }
		        // use_time = getClk();
		        printf("Clock Now: %d\n", getClk());
		        printf("Temp Data Count: %d\n", tempDataQueue.rear);
		        // sleep(1);
		    }
			else
			{
				// increment timer for exiting 
				waiting_interval++;
				sleep(1);
				// printf("Waiting ... \n");
			}
			// check here to see if the scheduler waited for too long
			// (more than 30 seconds with no incoming processes
			if (waiting_interval >= 30)
			{
				// Closing the log file
				fclose(logFilePtr);
				printf("Waited for too long. Exiting ... \n");
				use_time = getClk() - waiting_interval;
				printf("total time: %d\nuse time: %d\n", total_time, use_time);
				printf("CPU Utilization: %0.2f\n", (double)(use_time/total_time)*100);
				// Outputting PERF File 
				fprintf(perfFilePtr, "CPU utilization: %0.2f\n", (double)((double)use_time/(double)total_time)*100);
				fclose(perfFilePtr);
				exit(4);
			}
		}
		use_time = getClk() - waiting_interval;
		printf("total time: %d\nuse time: %d\n", total_time, use_time);
		printf("CPU Utilization: %0.2f\n", (double)((double)use_time/(double)total_time)*100);
		// Outputting PERF File 
		fprintf(perfFilePtr, "CPU utilization: %lf\n", (double)((double)use_time/(double)total_time)*100);
		fclose(perfFilePtr);
		printf("Exiting Scheduler ... \n");
		int serv_status;
		serv_pid = wait(&serv_status);
		if (serv_pid >= 0)
		{
			if (WEXITSTATUS(serv_status) == 4)
			{
				// Scheduler terminated normally
				exit(0);
			}
		}
	}
}

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
            WaitTime = getClk() - runningProcess.arrival;
            fprintf(logFilePtr, "At time %d process %d finished arr %d total %d remain %d wait %d TA %d WTA %0.2f\n",
                                getClk(),runningProcess.id, runningProcess.arrival, runningProcess.runtime, 0, WaitTime, (getClk() - runningProcess.arrival) /*TA*/, (getClk() - runningProcess.arrival)/ runningProcess.runtime /*WTA*/); kill(runningProcess.id, SIGCONT);
            available = false;
        }

        sleep(1);

    }
    //upon termination release the clock resources
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
                        runningProcess.remainingT = runningProcess.runtime - clock()/CLOCKS_PER_SEC;
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
        sleep(1); 
    }    
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
    signal(SIGCHLD, hpfHandler);
    signal(SIGINT,clearResources);
    
    initClk();
    msgqid1 = msgget(16499, 0644);

    logFilePtr = fopen("scheduler.log", "w");
    if (logFilePtr == NULL)
    {
        printf("Unable to create log file for the scheduler ... \n");
    }
    
    perfFilePtr = fopen("scheduler.perf", "w");
    if (perfFilePtr == NULL)
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
    else if (strcmp(argv[1], "-RR") == 0)
    {
        // Create a shared memory for the ready queue between child and parent
        int shm_fd;
        struct Queue * shared_processes_memory;
        const char *shared_memory_name = "ProcessesData";
        // free up shared memory (if it exists)
        shm_unlink(shared_memory_name);
        int msize = sizeof(struct Queue);	// size of the shared memory segment (in bytes)
        // open the memory
        shm_fd = shm_open(shared_memory_name, O_CREAT | O_EXCL | O_RDWR, S_IRWXU | S_IRWXG);
        if (shm_fd < 0)
        {
	        fprintf(stderr, "Error in shm_open()");
	        return;
        }
        // attach the shared memory segment 
        ftruncate(shm_fd, msize);
        // allocating the shared memory segment
        shared_processes_memory = (struct Queue *)mmap(NULL, msize, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
        if (shared_processes_memory == NULL)
        {
	        fprintf(stderr, "Error in mmap()");
	        return;
        }


        // Create a shared memory for the quanta between child and parent
        int shm_fd1;
        int * shared_quanta_memory;
        const char *shared_memory_name1 = "QuantaData";
        // free up shared memory (if it exists)
        shm_unlink(shared_memory_name1);
        int msize1 = sizeof(int);	// size of the shared memory segment (in bytes)
        // open the memory
        shm_fd1 = shm_open(shared_memory_name1, O_CREAT | O_EXCL | O_RDWR, S_IRWXU | S_IRWXG);
        if (shm_fd1 < 0)
        {
	        fprintf(stderr, "Error in shm_open()");
	        return;
        }
        // attach the shared memory segment 
        ftruncate(shm_fd1, msize1);
        // allocating the shared memory segment
        shared_quanta_memory = (int *)mmap(NULL, msize1, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd1, 0);
        printf("RR Scheduler Initialized\n");
        RR_Schedule(shared_quanta_memory, shared_processes_memory);
    }


    // scheduler logic
    
    
}
void hpfHandler(int signum){
    printf("Process %d has finished at time %d\n", runningProcess.id, getClk());
    available = true;
    signal(SIGCHLD, hpfHandler);
}
void clearResources(int signum){
    msgctl(msgqid1, IPC_RMID, (struct msqid_ds *) 0);
    // close the file 
	fclose(logFilePtr);
	printf("Saving log file ... \n");
    exit(0);
}
