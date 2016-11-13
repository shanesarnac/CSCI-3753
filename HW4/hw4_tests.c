
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sched.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define TOTAL_PROCESSES 1000

char debug = 0;

enum Process_Type {
	Compute_Bound,
	IO_Bound,
	Mixed
};

int main(int argc, char* argv[]) {
	if (debug) {
		printf("HW4 Tests Initiated\n");
		printf("argc = %d\n", argc);
	}
	int i, policy, processes, priority_min, priority_max;
	char random_priority;
	struct sched_param param;
	enum Process_Type process_type;
	

	if(argc > 1){
		if (debug) {
			printf("First argument: %s\n", argv[1]);
		}
		
		if(!strcmp(argv[1], "SCHED_OTHER")){
		    policy = SCHED_OTHER;
		}
		else if(!strcmp(argv[1], "SCHED_FIFO")){
		    policy = SCHED_FIFO;
		}
		else if(!strcmp(argv[1], "SCHED_RR")){
		    policy = SCHED_RR;
		}
		else{
		    fprintf(stderr, "Unhandeled scheduling policy\n");
		    exit(EXIT_FAILURE);
		}
    }
    else {
		policy = SCHED_OTHER;
	}
    
    if(argc > 2) {
		if (debug) {
			printf("Second argument: %s\n", argv[2]);
		}
		
		processes = atol(argv[2]);
		if (processes < 1) {
			fprintf(stderr, "Bad processes value\n");
			exit(EXIT_FAILURE);
		}
	}
	else {
		processes = TOTAL_PROCESSES;
	}
	
	if (argc > 3) {
		if (debug) {
			printf("Third argument: %s\n", argv[3]);
		}
		
		if(!strcmp(argv[3], "CPU")) {
			process_type = Compute_Bound;
		}
		else if(!strcmp(argv[3], "IO")) {
			process_type = IO_Bound;
		}
		else {
			process_type = Mixed;
		}
	}
	
	if (argc > 4) {
		if (debug) {
			printf("Fourth argument: %s\n", argv[4]);
		}
		
		if(!strcmp(argv[4], "RAND")) {
			random_priority = 1;
		}
		else {
			random_priority = 0;
		}
	}
	else {
		random_priority = 0;
	}
	
	pid_t pid;
	pid_t pids[processes];
	
	priority_min = sched_get_priority_min(policy);
	priority_max = sched_get_priority_max(policy);
	
	if (random_priority && (policy == SCHED_FIFO || policy == SCHED_RR)) {
		if (debug) {
			printf("Random Priority and either SCHED_FIFO or SCHED_RR\n");
		}
		
		// Set a random priority for each forked process running under
		// SCHED_FIFO or SCHED_RR policies. 
		time_t t;
		srand((unsigned) time(&t));
		
		for (i = 0; i < processes; i++) {
		    param.sched_priority = rand()%(priority_max - priority_min) + 1;
		    if(sched_setscheduler(0, policy, &param)){
				perror("Error setting scheduler policy");
				exit(EXIT_FAILURE);
		    }
		    
			pid = fork();
			if (pid == 0) {
				break;
			}
			else {
				pids[i] = pid;
			}
		}
		
	}
	else if(random_priority && policy == SCHED_OTHER) {
		// Set a random nice value for each forked process running under
		// SCHED_OTHER
		// niceness range: -20 to 19
		if (debug) {
			printf("Random Priority and SCHED_OTHER\n");
		}
		
		int range = 39;
		int niceness;
		for (i = 0; i < processes; i++) {
			niceness = (rand()%(range+1)) - 20;
			setpriority(PRIO_PROCESS, 0, niceness);
			
			pid = fork();
			if (pid == 0) {
				break;
			}
			else {
				pids[i] = pid;
			}
		}
		
	}
	else {
		if (debug) {
			printf("Uniform Priority\n");
		}

		// Maintain uniform priorities and niceness for all forked processes. 
		param.sched_priority = sched_get_priority_max(policy);
	    if(sched_setscheduler(0, policy, &param)){
			perror("Error setting scheduler policy");
			exit(EXIT_FAILURE);
	    }
	    
		for (i = 0; i < processes; i++) {;
			pid = fork();
			if (pid == 0) {
				break;
			}
			else {
				pids[i] = pid;
			}
		}
	}
	
	if (pid == 0 && process_type == Compute_Bound) {
		if (debug) {
			printf("Compute Bound process\n");
		}
		
		// Compute (CPU) Bound Process
		char *const *args = {NULL};
		execv("pi", args);
	}
	else if (pid == 0 && process_type == IO_Bound) {
		if (debug) {
			printf("IO Bound process\n");
		}
		
		char *const *args = {NULL};
		execv("rw", args);
	}
	else if (pid == 0 && process_type == Mixed) {
		if (debug) {
			printf("Mixed process\n");
		}
	}
	else {
		if (debug) {
			printf("Parent beginning to wait for PIDS to complete execution\n");
		}
		
		// Parent now waits for all children to finish. 
		for (i = 0; i < TOTAL_PROCESSES; i++) {
			waitpid(pids[i], NULL, 0);
		}
	}
	
	if (debug) {
		printf("Exiting program");
	}
	
	return 0;
}
