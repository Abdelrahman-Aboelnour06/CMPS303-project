build:
	gcc -g process_generator.c Processes_DataStructure/process_queue.c Processes_DataStructure/process_priority_queue.c Processes_DataStructure/process.h -o process_generator.out
	gcc -g clk.c -o clk.out
	gcc -g scheduler.c Processes_DataStructure/process_queue.c Processes_DataStructure/process_priority_queue.c Processes_DataStructure/process.h -o scheduler.out
	gcc -g process.c -o process.out
	gcc -g test_generator.c -o test_generator.out

clean:
	rm -f *.out  processes.txt

all: clean build

run:
	./test_generator.out
	./process_generator.out processes.txt 
