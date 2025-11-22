#include "headers.h"
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

int main(int agrc, char * argv[])
{
    initClk();

    int remainTime = atoi(argv[1]);
    int ID = atoi(argv[2]);
    printf("Process %d with remaining time %d started at time %d\n", ID, remainTime, getClk());
    while (remainTime > 0)
    {
        remainTime--;
        sleep(1);
    }
    printf("Process %d stopped at time %d\n", ID, getClk());

    exit(0);
}