#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "util.h"
#include "queue.h"


#define MINARGS 3
#define SBUFSIZE 1025
#define INPUTFS "%1024s"
#define USAGE "<inputFilePath> <outputFilePath>"
#define QUEUE_MAX 100

#define NUM_THREADS	5

// Global variables defined
queue q;
char debug = 0;
pthread_mutex_t queue_access;
pthread_mutex_t output_file_access;
pthread_cond_t queue_full;

typedef struct {
	char hostname[SBUFSIZE];
    char firstipstr[INET6_ADDRSTRLEN];
} Map_IP;



// write to file
void *writeFile(void* output_file_ptr) {
	if (debug) {
		printf("Entered writeFile\n");
	}
	
	
	FILE* outputfp = (FILE *) output_file_ptr;
	
	Map_IP *full_info = NULL;
	
	// Get element from queue
	pthread_mutex_lock(&queue_access);
	
	// Queue is empty
	while (queue_is_empty(&q)) {
		pthread_cond_wait(&queue_full, &queue_access);
	}
	
	full_info = queue_pop(&q); 
	
	pthread_mutex_unlock(&queue_access);
	
	
	// Write to output file
	pthread_mutex_lock(&output_file_access);
	
	fprintf(outputfp, "%s,%s\n", full_info->hostname, full_info->firstipstr);
	
	pthread_mutex_unlock(&output_file_access);
	
	free(full_info);
	
	if (debug) {
		printf("finished writing to file\n");
	}
	
	return NULL;
}


// read from file
void *readFile(void* input_file_ptr) {
	
	if (debug) {
		printf("Entered readFile\n");
	}
	
	FILE* inputfp = (FILE*) input_file_ptr;
	char hostname[SBUFSIZE];
	
	while (fscanf(inputfp, INPUTFS, hostname) > 0) {
		Map_IP *full_info = malloc(sizeof(Map_IP));
		strncpy(full_info->hostname, hostname, sizeof(hostname));
		
		if (debug) {
			printf("The first entry in the file is: %s\n", full_info->hostname);
		}
		
		if (dnslookup(full_info->hostname, full_info->firstipstr, sizeof(full_info->firstipstr)) == UTIL_FAILURE) {
			fprintf(stderr, "dnslookup error: %s\n", full_info->hostname);
			strncpy(full_info->firstipstr, "", sizeof(full_info->firstipstr));
		}
		
		if (debug) {
			printf("It's IP address is: %s\n", full_info->firstipstr);
		}
		
		// Add to queue
		pthread_mutex_lock(&queue_access);
		
		// The Queue is full
		if (queue_is_full(&q)) {
			pthread_cond_wait(&queue_full, &queue_access);
		}
		
		queue_push(&q, (void *) full_info);
		
		pthread_mutex_unlock(&queue_access);
		pthread_cond_signal(&queue_full);
		
	}
	
	if (debug) {
		printf("finished reading in file\n");
	}
	
	return NULL;
}


int main(int argc, char *argv[])
{
	if (argc < MINARGS) {
		fprintf(stderr, "Not enough arguments: %d\n", (argc - 1));
		fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
	}
	
	// Declare needed variables
	FILE* outputfp = NULL;
	FILE* inputfp[argc-2];
	pthread_t requester_threads[argc-2]; // one thread per file
	pthread_t resolver_threads[argc-2]; // one thread per file
	char errorstr[SBUFSIZE];
	int i, rc;
	
	if(queue_init(&q, QUEUE_MAX) == QUEUE_FAILURE) {
		fprintf(stderr,"error: queue_init failed!\n");
	}
	
	// Initialize semaphores and condition variables
	rc = pthread_mutex_init(&queue_access, NULL); 
	if (rc < 0) {
		perror("Error Inititializing Semaphore");
	}
	
	rc = pthread_mutex_init(&output_file_access, NULL);
	if (rc < 0) {
		perror("Error Inititializing Semaphore");
	}
	
	rc = pthread_cond_init(&queue_full, NULL);
	if (rc < 0) {
		perror("Error Inititializing Condition Variable");
	}
	
	
	outputfp = fopen(argv[(argc-1)], "w");
    if(!outputfp){
		perror("Error Opening Output File");
		return EXIT_FAILURE;
    }
	
	// Open each input file and send it on its merry way with a thread
	for (i = 1; i < argc-1; i++) {
		inputfp[i-1] = fopen(argv[i], "r");
		if(!inputfp[i-1]){
		    sprintf(errorstr, "Error Opening Input File: %s", argv[i]);
		    perror(errorstr);
		    break;
		}	
		
		// Create Requester Threads
		rc = pthread_create(&(requester_threads[i-1]), NULL, readFile, (void *) inputfp[i-1]);
		if (rc) {
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(EXIT_FAILURE);
		}
		
		// Create Resolver Threads
		rc = pthread_create(&(resolver_threads[i-1]), NULL, writeFile, (void *) outputfp);
		if (rc) {
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(EXIT_FAILURE);
		}
		
	}
	
	 // All threads have finished executing
	for (i = 0; i < argc-2; i++) {
		pthread_join(requester_threads[i], NULL);
		pthread_join(resolver_threads[i], NULL);
		fclose(inputfp[i]);
	}
   
	// Clean memory
    queue_cleanup(&q);
    fclose(outputfp);
    
    // de-initialize semaphores and condition variables
    pthread_mutex_destroy(&queue_access);
    pthread_mutex_destroy(&output_file_access);
    pthread_cond_destroy(&queue_full);

    return 0;
}
