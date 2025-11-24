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

void myHandler(int signum){
    int status;
    int finishedPid = wait(&status);
    PCB* finished = getPcbByPid(finishedPid);
    if(finished == NULL) return;

    finished->process_state = Finished;
    finished->is_completed = true;
    finished->FINISH_TIME = getClk();
    runningPcb = NULL;
    pcbCount--;

    pFile = fopen("scheduler_log.txt", "a");
    fprintf(pFile, "At time %-5d process %-5d finished arr %-5d total %-5d remain %-5d wait %-5d TA %-5d WTA %.2f\n",
            getClk(), finished->process_id,
            finished->arrival_time,
            finished->RUNNING_TIME,
            finished->REMAINING_TIME,
            finished->FINISH_TIME - finished->arrival_time - finished->RUNNING_TIME,
            finished->FINISH_TIME - finished->arrival_time,
            (float)(finished->FINISH_TIME - finished->arrival_time)/finished->RUNNING_TIME);
    fclose(pFile);

    // Update dependent PCBs
    for (int i = 0; i < pcbCount; i++) {
        if (pcbArray[i]->dependency_id == finished->process_id) {
            // Dependency resolved
            pcbArray[i]->dependency_id = -1;
        }
    }
}

void runPcb(PCB* p) {
    if (p->STARTED == false) {
        int pid = fork();
        if (pid == 0) { // child
            char timeArg[16];
            sprintf(timeArg, "%d", p->REMAINING_TIME);
            execl("./process.out", "./process.out", timeArg, NULL);
            exit(1);
        } else { // parent
            p->process_pid = pid;
            p->STARTED = true;
            p->process_state = Running;
            p->START_TIME = getClk();
            p->LAST_EXECUTED_TIME = getClk();
        }

        pFile = fopen("scheduler_log.txt", "a");
        fprintf(pFile, "At time %-5d process %-5d started arr %-5d total %-5d remain %-5d wait %-5d\n",
                getClk(), p->process_id,
                p->arrival_time,
                p->RUNNING_TIME,
                p->REMAINING_TIME,
                getClk() - p->arrival_time);
        fclose(pFile);
    }
    else if (p->STARTED == true) {
        printf("this process need to continue\n");
        kill(p->process_pid, SIGCONT);
        p->process_state = Running;
        p->LAST_EXECUTED_TIME = getClk();
        p->WAITING_TIME += getClk() - p->LAST_EXECUTED_TIME;

        pFile = fopen("scheduler_log.txt", "a");
        fprintf(pFile, "At time %-5d process %-5d resumed arr %-5d total %-5d remain %-5d wait %-5d\n",
                getClk(), p->process_id,
                p->arrival_time,
                p->RUNNING_TIME,
                p->REMAINING_TIME,
                getClk() - p->arrival_time - (p->RUNNING_TIME - p->REMAINING_TIME));
        fclose(pFile);
    }
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
    while(1) {
        int rec_status = msgrcv(MESSAGE_ID,&PROCESS_MESSAGE, sizeof(message_buf),2,IPC_NOWAIT);
        if(rec_status!=-1) {
            switch(selected_Algorithm_NUM) {
                case 1: {
                    // HPF
                    pcbArray[pcbCount] = malloc(sizeof(PCB));
                    PCB *p = pcbArray[pcbCount++];
                    p->process_id = PROCESS_MESSAGE.p.ID;
                    p->priority = PROCESS_MESSAGE.p.PRIORITY;
                    p->dependency_id = PROCESS_MESSAGE.p.DEPENDENCY_ID;
                    p->REMAINING_TIME = PROCESS_MESSAGE.p.RUNNING_TIME;
                    p->RUNNING_TIME = PROCESS_MESSAGE.p.RUNNING_TIME;
                    p->arrival_time = PROCESS_MESSAGE.p.ARRIVAL_TIME;
                    p->STARTED = false;
                    p->is_completed = false;
                    p->process_state = Ready;
                    enqueuePriority(&readyPriorityQueue, p);
                    break;
                }
                case 2:
                    // SRTN
                    enqueue_priority_SRTN(&READY_PRIORITY_QUEUE, PROCESS_MESSAGE.p);
                    PRINT_READY_PRIORITY_QUEUE();
                    break;
                case 3: {
                    // RR
                    enqueue(&READY_QUEUE, PROCESS_MESSAGE.p);
                    PRINT_READY_QUEUE();
                    if(running_process_index == -1 && peek_front(&READY_QUEUE)->Process.first_time) {
                        int current_time = getClk();
                        
                        peek_front(&READY_QUEUE)->Process.first_time = false;
                        pcb[process_count].arrival_time = peek_front(&READY_QUEUE)->Process.ARRIVAL_TIME;
                        pcb[process_count].process_id = peek_front(&READY_QUEUE)->Process.ID;
                        pcb[process_count].RUNNING_TIME = peek_front(&READY_QUEUE)->Process.RUNNING_TIME;
                        pcb[process_count].REMAINING_TIME = peek_front(&READY_QUEUE)->Process.RUNNING_TIME;
                        pcb[process_count].START_TIME = current_time;
                        pcb[process_count].LAST_EXECUTED_TIME = current_time;
                        pcb[process_count].process_state = Running;
                        
                        char str_rem_time[20];
                        sprintf(str_rem_time, "%d", peek_front(&READY_QUEUE)->Process.RUNNING_TIME);
                        int pid = fork();
                        if(pid == 0) {
                            execl("./process.out", "./process.out", str_rem_time, NULL);
                            exit(0);
                        }
                        pcb[process_count].process_pid = pid;
                        running_process_index = process_count;
                        pcb[running_process_index].LAST_EXECUTED_TIME = getClk();
                        process_count++;
                        
                        pFile = fopen("scheduler_log.txt", "a");
                        fprintf(pFile, "At time %-5d process %-5d started arr %-5d total %-5d remain %-5d wait %-5d\n",
                                getClk(), pcb[running_process_index].process_id,
                                pcb[running_process_index].arrival_time,
                                pcb[running_process_index].RUNNING_TIME,
                                pcb[running_process_index].REMAINING_TIME,
                                current_time - pcb[running_process_index].arrival_time);
                        fclose(pFile);
                        
                        clock_timer = current_time; 
                        break;
                    }
                }
                default: break;
            }
        }

        







        if (selected_Algorithm_NUM == 1) {
            // Preemption: check if front of queue has higher priority than running
            if (runningPcb != NULL && !isPriorityQueueEmpty(&readyPriorityQueue)) {
                PCB* frontPcb = peekPriorityFront(&readyPriorityQueue);
                if (frontPcb->priority < runningPcb->priority) {
                    kill(runningPcb->process_pid, SIGSTOP);
                    runningPcb->REMAINING_TIME = getClk() - runningPcb->LAST_EXECUTED_TIME;
                    enqueuePriority(&readyPriorityQueue, runningPcb);
                    
                    pFile = fopen("scheduler_log.txt", "a");
                    fprintf(pFile, "At time %-5d process %-5d stopped arr %-5d total %-5d remain %-5d wait %-5d\n",
                        getClk(), runningPcb->process_id,
                        runningPcb->arrival_time,
                        runningPcb->RUNNING_TIME,
                        runningPcb->REMAINING_TIME,
                        getClk() - runningPcb->arrival_time - (runningPcb->RUNNING_TIME - runningPcb->REMAINING_TIME));
                        fclose(pFile);
                        
                        
                        runningPcb = dequeuePriority(&readyPriorityQueue);
                    runPcb(runningPcb);
                }
            }

            // No current running process
            if (runningPcb == NULL && !isPriorityQueueEmpty(&readyPriorityQueue)) {
                runningPcb = dequeuePriority(&readyPriorityQueue);
                // Dependency check
                if (runningPcb->dependency_id != -1) {
                    PCB* depPcb = getPcbById(runningPcb->dependency_id);
                    if (depPcb && !depPcb->is_completed) {
                        // Give dependency runnng priority-1 so it can finish first
                        updatePriority(&readyPriorityQueue, depPcb, runningPcb->priority-1);
                        runningPcb = NULL; // wait for dependency
                        continue;
                    }
                }

                // Run the process
                printf("before:\n");
                printPriorityQueue(&readyPriorityQueue);
                runPcb(runningPcb);
                printf("after:\n");
                printPriorityQueue(&readyPriorityQueue);
            }
        }













        if(selected_Algorithm_NUM==3 && clock_timer!=getClk()){
        clock_timer=getClk();
        if(peek_front(&READY_QUEUE)==NULL) continue;
        PRINT_READY_QUEUE();
        
        int process_index=get_pcb_index(pcb,process_count,peek_front(&READY_QUEUE)->Process.ID);
        
            if(process_index==-1 && peek_front(&READY_QUEUE)->Process.first_time){
                peek_front(&READY_QUEUE)->Process.first_time=false;
                pcb[process_count].arrival_time=peek_front(&READY_QUEUE)->Process.ARRIVAL_TIME;
                pcb[process_count].process_id=peek_front(&READY_QUEUE)->Process.ID;
                pcb[process_count].RUNNING_TIME=peek_front(&READY_QUEUE)->Process.RUNNING_TIME;
                pcb[process_count].REMAINING_TIME=peek_front(&READY_QUEUE)->Process.RUNNING_TIME;
                pcb[process_count].START_TIME=getClk();
                pcb[process_count].LAST_EXECUTED_TIME=getClk();
                pcb[process_count].process_state=Running;
                
                char str_rem_time[20];
                sprintf(str_rem_time,"%d",peek_front(&READY_QUEUE)->Process.RUNNING_TIME);
                int pid=fork();
                if(pid==0){
                    execl("./process.out","./process.out",str_rem_time,NULL);
                    exit(0);
                }
                pcb[process_count].process_pid=pid;
                running_process_index=process_count;
                process_count++;
                
                pFile = fopen("scheduler_log.txt", "a");
                fprintf(pFile, "At time %-5d process %-5d started arr %-5d total %-5d remain %-5d wait %-5d\n",
                        getClk(), pcb[running_process_index].process_id,
                        pcb[running_process_index].arrival_time,
                        pcb[running_process_index].RUNNING_TIME,
                        pcb[running_process_index].REMAINING_TIME,
                        getClk() - pcb[running_process_index].arrival_time);
                fclose(pFile);
                
            } else {
            //
            if(running_process_index!=-1 && pcb[running_process_index].process_state == Running){
                ;
                
                int time_executed=getClk()-pcb[running_process_index].LAST_EXECUTED_TIME;
                
                if(time_executed>=TIME_QUANTUM && get_count(&READY_QUEUE)>1){
                    kill(pcb[running_process_index].process_pid,SIGSTOP);
                    pcb[running_process_index].process_state=Ready;
                    
                    pFile = fopen("scheduler_log.txt", "a");
                    fprintf(pFile, "At time %-5d process %-5d stopped arr %-5d total %-5d remain %-5d wait %-5d\n",
                            getClk(), pcb[running_process_index].process_id,
                            pcb[running_process_index].arrival_time,
                            pcb[running_process_index].RUNNING_TIME,
                            pcb[running_process_index].REMAINING_TIME,
                            getClk() - pcb[running_process_index].arrival_time - 
                            (pcb[running_process_index].RUNNING_TIME - pcb[running_process_index].REMAINING_TIME));
                    fclose(pFile);
                    
                    process_Node* temp=dequeue(&READY_QUEUE);
                    enqueue(&READY_QUEUE,temp->Process);
                    
                    int next_process_index = get_pcb_index(pcb, process_count, peek_front(&READY_QUEUE)->Process.ID);
                    
                    if(next_process_index == -1 && peek_front(&READY_QUEUE)->Process.first_time){
                        peek_front(&READY_QUEUE)->Process.first_time=false;
                        pcb[process_count].arrival_time=peek_front(&READY_QUEUE)->Process.ARRIVAL_TIME;
                        pcb[process_count].process_id=peek_front(&READY_QUEUE)->Process.ID;
                        pcb[process_count].RUNNING_TIME=peek_front(&READY_QUEUE)->Process.RUNNING_TIME;
                        pcb[process_count].REMAINING_TIME=peek_front(&READY_QUEUE)->Process.RUNNING_TIME;
                        pcb[process_count].START_TIME=getClk();
                        pcb[process_count].LAST_EXECUTED_TIME=getClk();
                        pcb[process_count].process_state=Running;
                        
                        char str_rem_time[20];
                        sprintf(str_rem_time,"%d",peek_front(&READY_QUEUE)->Process.RUNNING_TIME);
                        int pid=fork();
                        if(pid==0){
                            execl("./process.out","./process.out",str_rem_time,NULL);
                            exit(0);
                        }
                        pcb[process_count].process_pid=pid;
                        running_process_index=process_count;
                        process_count++;
                        

                        pFile = fopen("scheduler_log.txt", "a");
                        fprintf(pFile, "At time %-5d process %-5d started arr %-5d total %-5d remain %-5d wait %-5d\n",
                                getClk(), pcb[running_process_index].process_id,
                                pcb[running_process_index].arrival_time,
                                pcb[running_process_index].RUNNING_TIME,
                                pcb[running_process_index].REMAINING_TIME,
                                getClk() - pcb[running_process_index].arrival_time);
                        fclose(pFile);
                        
                    } else if(next_process_index != -1){
                        running_process_index = next_process_index;
                        kill(pcb[running_process_index].process_pid,SIGCONT);
                        pcb[running_process_index].LAST_EXECUTED_TIME=getClk();
                        pcb[running_process_index].process_state=Running;
                        
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
                }
                else{
                    pcb[running_process_index].REMAINING_TIME--;
                }
            }
        }
    }
    }

    destroyClk(true);
}