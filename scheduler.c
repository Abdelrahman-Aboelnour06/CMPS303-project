#include "Processes_DataStructure/process.h"
#include "Processes_DataStructure/process_priority_queue.h"
#include "Processes_DataStructure/process_queue.h"
#include "headers.h"
//#include <cstddef>
#include <signal.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <unistd.h>

/*---------------------------------Omar Syed------------------------------------*/

int MESSAGE_ID;
/*
1-Receive processes
2-Initialize Queues & PCB
3-Initialize Algorithms
Done by Omar Syed
*/
/*---------------------------------QUEUES&PCB------------------------------------*/


// handle process termination
process *curProcess = NULL;
PCB *pcbHead = NULL;
void childHandler(int SIGNUM) {
    int status;
    int childPID = wait(&status);

    //update its PCB
    PCB *pcb = GET_PCB(pcbHead, childPID);
    pcb->process_state = Finished;
    pcb->is_completed = true;
    pcb->FINISH_TIME = getClk();
    pcb->REMAINING_TIME = 0;

    //current now is null
    curProcess = NULL;
}

//cpu bound --> no blocking queue
PCB PCB_ENTRY;
process_queue READY_QUEUE;
process_priority_queue READY_PRIORITY_QUEUE;


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

int process_count=0;
int PID[max];
int j =0;
/*---------------------------------Omar Syed------------------------------------*/
int main(int argc, char * argv[])
{
    int clock_timer= 0;
    initClk();
    
    //TODO implement the scheduler :)
    /*---------------------------Omar Syed------------------------------------*/
    initialize_queue(&READY_QUEUE);
    initialize_priority_queue(&READY_PRIORITY_QUEUE);
    int selected_Algorithm_NUM=atoi(argv[1]);
    int TIME_QUANTUM=atoi(argv[2]);// if RR 3
    key_t key_msg_process = ftok("keyfile", 'A');
    MESSAGE_ID = msgget(key_msg_process, 0666|IPC_CREAT);
    printf("queue id  is: %d\n",MESSAGE_ID);
    if(MESSAGE_ID==-1){
        printf("Error In Creating Message Queue!\n");
    }
    message_buf PROCESS_MESSAGE;

    // -=-=-=-=--=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--=-=-=-=-=-=-=-=-=-=-=-=-
    // if algorthim is HPF
    signal(SIGCHLD, childHandler);
    process_priority_queue *queue = malloc(sizeof(process_priority_queue));
    initialize_priority_queue(queue);
    while (1) {
        //check for ready processes + preempte if higher priority
        printf("check1, process%d\n\n", j);
        int status = msgrcv(MESSAGE_ID,&PROCESS_MESSAGE, sizeof(message_buf),2,IPC_NOWAIT);
        printf("%d\n", status);
        if(status != -1) {
            enqueue_priority(queue, PROCESS_MESSAGE.p); //enqueue new process
            printf("check2, process%d\n\n", j);
            //preemption check
            if (curProcess != NULL && (peek_priority_front(queue)->PRIORITY < curProcess->PRIORITY)) {
                //stop current
                printf("check3, process%d\n\n", j);
                kill(curProcess->ID, SIGSTOP);
                
                //update current pcb
                PCB *pcb = GET_PCB(pcbHead, curProcess->ID);
                pcb->process_state = Ready;
                pcb->REMAINING_TIME -= (getClk() - pcb->LAST_EXECUTED_TIME);
                pcb->LAST_EXECUTED_TIME = getClk();
                
                //enqueue current
                enqueue_priority(queue, *curProcess);

                //current = NULL, if below will handle the higher priority
                curProcess = NULL;
                printf("check4, process%d\n\n", j);
            }
        }
        printf("check5, process%d\n\n", j);
        //run a process from ready queue if no process is running already
        if (curProcess == NULL && !is_priority_queue_empty(queue)) {
            printf("check6, process%d\n\n", j);
            curProcess = dequeue_priority(queue);
            PCB *pcb = GET_PCB(pcbHead, curProcess->ID);

            //if already started
            if (pcb) {
                //update resumed pcb + continue it
                pcb->process_state = Running;
                pcb->WAITING_TIME += (getClk() - pcb->LAST_EXECUTED_TIME);
                kill(curProcess->ID, SIGCONT);
                pcb->LAST_EXECUTED_TIME = getClk();
            }
            //if first time to run
            else {
                printf("check7, process%d\n\n", j);
                pcb = malloc(sizeof(PCB));
                INITIALIZE_PCB(pcb);
                pcb->p = *curProcess;
                pcb->process_state = Running;
                pcb->START_TIME = getClk();
                pcb->LAST_EXECUTED_TIME = getClk();
                pcb->REMAINING_TIME = curProcess->RUNNING_TIME;
                pcb->WAITING_TIME = 0;
                pcb->is_completed = false;
                enqueue_PCB(&pcbHead, pcb);

                int pid = fork();
                if (pid == 0) {
                    char remainTime[16];
                    char ID[16];
                    sprintf(remainTime, "%d", pcb->REMAINING_TIME);
                    sprintf(ID, "%d", curProcess->ID);
                    if (execl("./process.out", "process.out", remainTime, ID, NULL) == -1) {
                        perror("execl failed");
                        exit(1);
                    }
                }
                else if (pid > 0) PID[process_count++] = pid;
                else {
                    perror("fork failed");
                    exit(1);
                }
            }
        }
        j++;
    }
    free_priority_queue(queue);
    dequeue_PCB(&pcbHead);
    // -=-=-=-=--=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--=-=-=-=-=-=-=-=-=-=-=-=-

    destroyClk(true);
}