## \# CMPS 303 Project: Operating Systems Simulation

## 

## \## Overview

## This repository contains the source code for the CMPS 303 Operating Systems course project. It is a comprehensive C-based simulation of an Operating System's CPU scheduler and Memory Management Unit (MMU). The system models the lifecycle of processes, context switching, IPC (Inter-Process Communication), and memory allocation.

## 

## \## Project Architecture

## The project is built in C and is divided into several core OS modules that interact to simulate a complete scheduling environment.

## 

## \### 1. Core OS Components

## \* \*\*Process Generator (`process\_generator.c`, `process.c`):\*\* Parses input files (`processes.txt`) and handles the creation, initialization, and dispatching of simulated processes to the scheduler.

## \* \*\*CPU Scheduler (`scheduler.c`, `scheduler2.c`):\*\* The central orchestrator that manages the ready queue, dispatches processes to the CPU, and handles context switching based on the active scheduling algorithm.

## \* \*\*Clock \& Timing (`clk.c`):\*\* Simulates the hardware clock and provides synchronization for process arrival and execution times.

## \* \*\*Memory Management Unit (`mmu.c`, `memory.h`):\*\* Simulates the allocation and deallocation of memory for processes as they arrive and terminate, managing requests from `requests\_1.txt`.

## 

## \### 2. Custom Data Structures (`Processes\_DataStructure/`)

## To effectively manage process states (Ready, Blocked, Running), the simulation uses custom data structures:

## \* \*\*Queues:\*\* Defined in `process\_queue.c` and `process\_queue.h` for algorithms like Round Robin.

## \* \*\*Priority Queues:\*\* Defined in `process\_priority\_queue.c` and `process\_priority\_queue.h` for priority-based algorithms (HPF, SRTN).

## 

## \## Supported Scheduling Algorithms

## The simulator implements and heavily tests three main CPU scheduling algorithms:

## \* \*\*Round Robin (RR):\*\* Time-slice based scheduling with support for variable time quanta (e.g., Quanta 1, Quanta 4).

## \* \*\*Shortest Remaining Time Next (SRTN):\*\* A preemptive algorithm that always schedules the process with the least remaining execution time.

## \* \*\*Highest Priority First (HPF):\*\* A priority-driven preemptive scheduler that includes advanced handling for process dependencies (chains, non-consecutive dependencies, and equal priority tie-breaking).

## 

## \## Testing and Scenarios

## The repository includes an extensive suite of test cases divided into folders based on the scheduling algorithm (`RR/`, `SRTN/`, `hpf/`). 

## \* \*\*Stress Testing:\*\* Includes scenarios for "Heavy Preemption," "Different Arrival Times," and "Adhoc" edge cases.

## \* \*\*Dependency Handling:\*\* The HPF folder specifically tests complex scenarios like single dependencies, chain dependencies, and equal priorities.

## \* \*\*Performance Metrics:\*\* The system evaluates scheduler performance and outputs the results (e.g., `scheduler.perf`).

## \* \*\*Test Generation:\*\* A `test\_generator.c` utility is included to dynamically generate randomized `processes.txt` workloads.

