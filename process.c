#include "headers.h"
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

void handler1(int signum) {
    printf("Process continues\n");
}

int main(int agrc, char * argv[])
{
    signal(SIGCONT, handler1);  // only valid handler

    initClk();

    int remainTime = atoi(argv[1]);
    int ID = atoi(argv[2]);
    printf("Process %d with remaining time %d started at time %d\n",
           ID, remainTime, getClk());

    while (remainTime > 0) {
        remainTime--;
        sleep(1);
    }

    printf("Process %d finished at time %d\n", ID, getClk());
    exit(ID);
}