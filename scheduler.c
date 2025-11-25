#include "Processes_DataStructure/process.h"
#include "Processes_DataStructure/process_priority_queue.h"
#include "Processes_DataStructure/process_queue.h"
#include "headers.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <unistd.h>

/*---------------------------------Omar Syed------------------------------------*/

int MESSAGE_ID;
/*
1-Receive processes
2-Initialize Queues & PCB
3-Initialize Algorithms
4-RR
Done by Omar Syed
*/
/*---------------------------------QUEUES&PCB------------------------------------*/

//cpu bound --> no blocking queue
process_queue READY_QUEUE;
process_priority_queue READY_PRIORITY_QUEUE;
int TIME_QUANTUM;
int pid[max];
int running_process_index=-1;
int process_count=0;
void PRINT_READY_QUEUE(){
    process_Node* temp=READY_QUEUE.front;
    printf("Ready Queue: ");
    while(temp!=NULL){
        printf("P%d ",temp->Process.ID);
        temp=temp->next;
        if(temp==READY_QUEUE.rear->next){
            break;
        }
    }
    printf("\n");
}

void PRINT_READY_PRIORITY_QUEUE(){
    process_PNode* temp=READY_PRIORITY_QUEUE.front;
    printf("Ready Priority Queue: ");
    while(temp!=NULL){
        printf("P%d ",temp->Process.ID);
        temp=temp->next;
    }
    printf("\n");
}


int get_count(process_queue* READY_QUEUE){
    process_Node* temp=READY_QUEUE->front;
    int count=0;
    while(temp!=NULL){
        count++;
        temp=temp->next;
        if(temp==READY_QUEUE->rear->next){
            break;
        }
    }
    return count;
}

//to be completed -omar

int finished_process=0;
int count_pid=-1;
PCB pcb[max];
FILE*pFile;

PCB* get_pcb(PCB*pcb,int process_count,int process_id){
    for(int i = 0 ;i<process_count;i++){
        if(pcb[i].process_id==process_id){
            return &pcb[i];
        }
    }
    return NULL;
}

PCB* get_pcb_pid(PCB*pcb,int process_count,int process_pid){
    for(int i = 0 ;i<process_count;i++){
        if(pcb[i].process_pid==process_pid){
            return &pcb[i];
        }
    }
    return NULL;
}

int get_pcb_index(PCB*pcb,int process_count,int process_id){
    for(int i = 0 ;i<process_count;i++){
        if(pcb[i].process_id==process_id){
            return i;
        }
    }
    return -1;
}

void remove_pcb(PCB*pcb,int *process_count,int process_id){
    int k= get_pcb_index(pcb,  *process_count,  process_id);
    for(int i=k;i<*process_count-1;i++){
        pcb[i]=pcb[i+1];
    }
    (*process_count)--;
}

void update_queue_RR(process_queue* READY_QUEUE){
    int index = get_pcb_index(pcb, process_count, peek_front(READY_QUEUE)->Process.ID);
    int current_time =getClk();
    if(current_time-pcb[index].LAST_EXECUTED_TIME>=TIME_QUANTUM){
        pcb[index].process_state=Ready;
         kill(pcb[index].process_pid, SIGSTOP);
        printf("Stopped\n");
        process_Node* temp =dequeue(READY_QUEUE);
        PRINT_READY_QUEUE();
        enqueue(READY_QUEUE, temp->Process);
        PRINT_READY_QUEUE();
    }
    else{
        pcb[index].REMAINING_TIME--;
    }
}


void handler(int signum){
    printf("Handler called - Process finished\n");
    
    if(peek_front(&READY_QUEUE) == NULL) {
        printf("Error: Queue is empty in handler\n");
        return;
    }
    
    int finished_id = peek_front(&READY_QUEUE)->Process.ID;
    PCB* finished = get_pcb(pcb, process_count, finished_id);
    
    if(finished == NULL) {
        printf("Error: Could not find PCB for process %d\n", finished_id);
        return;
    }
    
    waitpid(finished->process_pid, NULL, 0);
    finished->process_state = Finished;
    finished->FINISH_TIME = getClk();
    finished->is_completed = true;
    finished->REMAINING_TIME = 0;

    pFile = fopen("scheduler_log.txt", "a");
    fprintf(pFile, "At time %-5d process %-5d finished arr %-5d total %-5d remain %-5d wait %-5d TA %-5d WTA %.2f\n",
            getClk(), finished->process_id,
            finished->arrival_time, finished->RUNNING_TIME,
            finished->REMAINING_TIME,
            finished->FINISH_TIME - finished->arrival_time - finished->RUNNING_TIME,
            finished->FINISH_TIME - finished->arrival_time,
            (float)(finished->FINISH_TIME - finished->arrival_time) / finished->RUNNING_TIME);
    fclose(pFile);

    // Remove from PCB array BEFORE dequeue
    remove_pcb(pcb, &process_count, finished_id);
    dequeue(&READY_QUEUE);
    finished_process++;

    if(peek_front(&READY_QUEUE) == NULL) {
        printf("No more processes in queue\n");
        running_process_index = -1;
        return;
    }
    
    PRINT_READY_QUEUE();
    
    // Check if next process already has PCB entry (was stopped before)
    int next_process_index = get_pcb_index(pcb, process_count, peek_front(&READY_QUEUE)->Process.ID);
    
    if(next_process_index != -1){
        // Resume existing process
        running_process_index = next_process_index;
        kill(pcb[running_process_index].process_pid, SIGCONT);
        pcb[running_process_index].LAST_EXECUTED_TIME = getClk();
        pcb[running_process_index].process_state = Running;
        printf("Resumed process %d\n", pcb[running_process_index].process_id);
        pFile = fopen("scheduler_log.txt", "a");
        fprintf(pFile, "At time %-5d process %-5d resumed arr %-5d total %-5d remain %-5d wait %-5d\n",
                getClk(), pcb[running_process_index].process_id,
                pcb[running_process_index].arrival_time,
                pcb[running_process_index].RUNNING_TIME,
                pcb[running_process_index].REMAINING_TIME,
                getClk() - pcb[running_process_index].arrival_time - 
                (pcb[running_process_index].RUNNING_TIME - pcb[running_process_index].REMAINING_TIME));
        fclose(pFile);
    }
    else if(peek_front(&READY_QUEUE)->Process.first_time){
        // Start new process
        peek_front(&READY_QUEUE)->Process.first_time = false;
        pcb[process_count].process_state = Running;
        pcb[process_count].process_id = peek_front(&READY_QUEUE)->Process.ID;
        pcb[process_count].RUNNING_TIME = peek_front(&READY_QUEUE)->Process.RUNNING_TIME;
        pcb[process_count].arrival_time = peek_front(&READY_QUEUE)->Process.ARRIVAL_TIME;
        pcb[process_count].REMAINING_TIME = peek_front(&READY_QUEUE)->Process.RUNNING_TIME;
        pcb[process_count].START_TIME = getClk();
        pcb[process_count].LAST_EXECUTED_TIME = getClk();
        
        char str_rem_time[20];
        sprintf(str_rem_time, "%d", peek_front(&READY_QUEUE)->Process.RUNNING_TIME);
        int pid = fork();
        if(pid == 0){
            execl("./process.out", "./process.out", str_rem_time, NULL);
            perror("Error in execl\n");
            exit(0);
        }
        else{
            pcb[process_count].process_pid = pid;
            running_process_index = process_count;
            process_count++;
            printf("Started new process %d\n", pcb[running_process_index].process_id);
            pFile = fopen("scheduler_log.txt", "a");
            fprintf(pFile, "At time %-5d process %-5d started arr %-5d total %-5d remain %-5d wait %-5d\n", 
                    getClk(), pcb[running_process_index].process_id,
                    pcb[running_process_index].arrival_time,
                    pcb[running_process_index].RUNNING_TIME, 
                    pcb[running_process_index].REMAINING_TIME,
                    getClk() - pcb[running_process_index].arrival_time);
            fclose(pFile);
        }
    }
}

// -=-=-=-=-
PCB* pcbArray[max];
int pcbCount = 0;
PCB* runningPcb = NULL;
PcbPriorityQueue readyPriorityQueue;
int timer = 0;
int childFinished = 0;

PCB* getPcbById(int processId) {
    for (int i = 0; i < pcbCount; i++)
        if (pcbArray[i]->process_id == processId)
            return pcbArray[i];
    return NULL;
}

PCB* getPcbByPid(int pid) {
    for (int i = 0; i < pcbCount; i++){
        if (pcbArray[i]->process_pid == pid){ /// problem is most probably here
            return pcbArray[i];
        }
    }
    return NULL;
}

void printPriorityQueue(PcbPriorityQueue* queue) {
    if (!queue || isPriorityQueueEmpty(queue)) {
        printf("Priority Queue is empty.\n");
        return;
    }

    printf("Priority Queue (front -> rear):\n");
    PcbNode* current = queue->front;
    while (current) {
        PCB* p = current->pcb;
        printf("P%d [prio=%d, dep=%d, remaining=%d] -> ",
               p->process_id, p->priority, p->dependency_id, p->REMAINING_TIME);
        current = current->next;
    }
    printf("NULL\n");
}

void printPCB(PCB* p) {
    if (!p) {
        printf("PCB = NULL\n");
        return;
    }

    printf("--------------------------------------------------\n");
    printf("PCB for process %d\n", p->process_id);
    printf("--------------------------------------------------\n");

    printf("Process PID        : %d\n", p->process_pid);
    printf("Priority           : %d\n", p->priority);
    printf("Dependency ID      : %d\n", p->dependency_id);

    printf("State              : ");
    switch (p->process_state) {
        case Ready:    printf("READY\n"); break;
        case Running:  printf("RUNNING\n"); break;
        case Finished: printf("FINISHED\n"); break;
        default:       printf("UNKNOWN\n"); break;
    }

    printf("Started?           : %s\n", p->STARTED ? "YES" : "NO");
    printf("Completed?         : %s\n", p->is_completed ? "YES" : "NO");

    printf("Arrival Time       : %d\n", p->arrival_time);
    printf("Start Time         : %d\n", p->START_TIME);
    printf("Last Exec Time     : %d\n", p->LAST_EXECUTED_TIME);
    printf("Finish Time        : %d\n", p->FINISH_TIME);

    printf("Requested Runtime  : %d\n", p->RUNNING_TIME);
    printf("Remaining Time     : %d\n", p->REMAINING_TIME);

    printf("--------------------------------------------------\n\n");
}

void printLog(PCB* p, char *string) 
{
    pFile = fopen("scheduler_log.txt", "a");

    if (strcmp(string, "finished") == 0)
    {
        int TA = p->FINISH_TIME - p->arrival_time;
        float WTA = (float)TA / p->RUNNING_TIME;
        int W = TA - p->RUNNING_TIME;

        fprintf(pFile,
                "At time %d process %d %s arrival %d total %d remain %d wait %d TA %d WTA %.2f\n",
                getClk(),
                p->process_id,
                string,
                p->arrival_time,
                p->RUNNING_TIME,
                p->REMAINING_TIME,
                W,
                TA,
                WTA
        );
    }
    else
    {
        fprintf(pFile,
                "At time %d process %d %s arrival %d total %d remain %d\n",
            getClk(),
                p->process_id,
                string,
                p->arrival_time,
                p->RUNNING_TIME,
                p->REMAINING_TIME
            );
    }

    fclose(pFile);
}

void runPcb(PCB* p, int currentTick) {
    if (!p->STARTED) {
        int pid = fork();
        if (pid == 0) { // child
            char timeArg[16];
            sprintf(timeArg, "%d", p->REMAINING_TIME);
            execl("./process.out", "./process.out", timeArg, NULL);
            exit(1);
        }

        // Update PCB
        p->STARTED = true;
        p->process_pid = pid;
        p->process_state = Running;
        p->START_TIME = currentTick;
        p->LAST_EXECUTED_TIME = currentTick;

        printLog(p, "started");
    } else {
        // Resume process
        kill(p->process_pid, SIGCONT);
        p->process_state = Running;
        p->LAST_EXECUTED_TIME = currentTick;

        printLog(p, "resumed");
    }
    printPCB(p);
}

bool depCheck(PCB* p) {
    // Case 1: No dependency
    if (p->dependency_id == -1)
        return true;

    // Get the PCB of the dependency
    PCB* depPcb = getPcbById(p->dependency_id);

    // Case 2: Dependency not arrived yet
    if (depPcb == NULL)
        return false;

    // Case 3: Dependency arrived but not finished
    if (depPcb->is_completed == false)
        return false;

    // Case 4: Dependency finished
    return true;
}

void hpfLoop(int currentTick) {
    timer = currentTick;

    // Preemption check
    if (runningPcb != NULL && !isPriorityQueueEmpty(&readyPriorityQueue)) {
        if (peekPriorityFront(&readyPriorityQueue)->priority < runningPcb->priority) {
            kill(runningPcb->process_pid, SIGSTOP);
            enqueuePriority(&readyPriorityQueue, runningPcb);
            printLog(runningPcb, "stopped");

            runningPcb = dequeuePriority(&readyPriorityQueue);

            printf("test1\n");
            if (depCheck(runningPcb)) {
                printf("test2\n");
                runPcb(runningPcb, timer);
            } else {
                printf("test3\n");
                enqueuePriority(&readyPriorityQueue, runningPcb);
                runningPcb = NULL;
            }
        }
    }

    // Picking next process
    if (runningPcb == NULL && !isPriorityQueueEmpty(&readyPriorityQueue)) {
        PCB* nextPcb = dequeuePriority(&readyPriorityQueue);
        printf("test11\n");
        if (depCheck(nextPcb)) {
            printf("test12\n");
            runningPcb = nextPcb;
            runPcb(runningPcb, timer);
        } else {
            printf("test13\n");
            enqueuePriority(&readyPriorityQueue, nextPcb);
            runningPcb = NULL;
        }
    }

    // Update remaining time
    if (runningPcb != NULL && runningPcb->REMAINING_TIME > 0) {
        runningPcb->REMAINING_TIME--;
        runningPcb->LAST_EXECUTED_TIME = timer;
    }
}

void myHandler(int signum){
    int status;
    int finishedPid = wait(&status);
    PCB* finished = getPcbByPid(finishedPid);
    if (!finished) return;

    int now = timer; // use the tick snapshot
    finished->FINISH_TIME = now;
    finished->REMAINING_TIME = 0;
    finished->process_state = Finished;
    finished->is_completed = true;
    printf("now is:%d, finishis:%d\n", now, finished->FINISH_TIME);

    printLog(finished, "finished");

    runningPcb = NULL;

    // Update dependent processes
    for (int i = 0; i < pcbCount; i++) {
        if (pcbArray[i]->dependency_id == finished->process_id)
            pcbArray[i]->dependency_id = -1;
    }

    childFinished = 1; // main loop will handle next scheduling
    printPCB(finished);
}

/*---------------------------------Omar Syed------------------------------------*/
int main(int argc, char * argv[])
{
    
    pFile = fopen("scheduler_log.txt", "a");
    if (!pFile) {
        printf("Error opening file.\n");
    }
     fprintf(pFile, "%-5s %-10s %-10s %-10s %-20s %-5s %-10s %-10s %-10s %-20s %-10s %-20s\n",
        "#At", "time", "x", "process", "y","state","arr","w","total","z","remain","wait");
        fclose(pFile);
    int clock_timer= 0;
    int total_process = atoi(argv[3]);
    initClk();
    
    //TODO implement the scheduler :)
    /*---------------------------Omar Syed------------------------------------*/
    initialize_queue(&READY_QUEUE);
    initialize_priority_queue(&READY_PRIORITY_QUEUE);
    int selected_Algorithm_NUM=atoi(argv[1]);
    TIME_QUANTUM=atoi(argv[2]);// if RR 3
    key_t key_msg_process = ftok("keyfile", 'A');
    MESSAGE_ID = msgget(key_msg_process, 0666|IPC_CREAT);
    printf("queue id  is: %d\n",MESSAGE_ID);
    if(MESSAGE_ID==-1){
        printf("Error In Creating Message Queue!\n");
    }
    message_buf PROCESS_MESSAGE;
    process_count=0;
    /*---------------------------Omar Syed------------------------------------*/
    
    
    if (selected_Algorithm_NUM == 1) {
        signal(SIGUSR1, myHandler);
    } else if (selected_Algorithm_NUM == 3) {
        signal(SIGUSR1, handler);
    }

    // printf("Scheduling Algorithm is Round Robin with Time Quantum = %d\n",TIME_QUANTUM);
    int firsttime =true; // TODO::fix this to be when queue is empty
    process_Node* current_process;
    while (1) {
        int now = getClk();

        if (now != timer) { // tick advanced
            timer = now;

            // Drain all incoming messages
            message_buf msg;
            while (msgrcv(MESSAGE_ID, &msg, sizeof(msg.p), 2, IPC_NOWAIT) != -1) {
    printf("Received process %d dep_id=%d\n", msg.p.ID, msg.p.DEPENDENCY_ID);

    // Allocate a single PCB for this process
    PCB* p = malloc(sizeof(PCB));
    memset(p, 0, sizeof(PCB));  // ensures no garbage values

    // Assign all fields
    p->process_id = msg.p.ID;
    p->priority = msg.p.PRIORITY;
    p->dependency_id = msg.p.DEPENDENCY_ID;  // assign AFTER initialization
    p->REMAINING_TIME = msg.p.RUNNING_TIME;
    p->RUNNING_TIME = msg.p.RUNNING_TIME;
    p->arrival_time = msg.p.ARRIVAL_TIME;
    p->STARTED = false;
    p->is_completed = false;
    p->process_state = Ready;

    // Save pointer in arrays / queues
    pcbArray[pcbCount++] = p;
    enqueuePriority(&readyPriorityQueue, p);

    // Debug prints
    printf("PCB created: process_id=%d dep_id=%d\n", p->process_id, p->dependency_id);
            }

            // Run scheduler
            if (!isPriorityQueueEmpty(&readyPriorityQueue) || runningPcb != NULL || childFinished)
                hpfLoop(timer);

            childFinished = 0; // reset flag
        }
    }

    destroyClk(true);
}