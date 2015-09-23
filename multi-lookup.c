/*
 * File: multi-lookup.h
 * Authors: Cameron Taylor and Steven Conflenti
 * Project: CSCI 3753 Programming Assignment 3
 * Date: 3/13/2015
 * Description: This file contains declarations of utility functions for
 *     the threaded DNS Name Resolution Engine.
 *  
 */

#include "multi-lookup.h"

#define NUM_THREADS	2
#define MINARGS 3
#define USAGE "<inputFilePath> <outputFilePath>"
#define SBUFSIZE 1025
#define INPUTFS "%1024s"

queue buffer;
pthread_mutex_t buffer_mutex;
pthread_mutex_t output_mutex;
pthread_mutex_t var_mutex;

int numOfThreads;

/* Function for Each Requester Thread to Run */
void* requestDNS(void* file){
    pthread_mutex_lock(&var_mutex);
    numOfThreads++;
    pthread_mutex_unlock(&var_mutex);
    
    FILE* inputFile = fopen((char*) file, "r");
    
    // if file unable to open, cleanup and exit
    if(!inputFile){
		fprintf(stderr, "Error opening input file: %s\n", (char *) file);
		pthread_mutex_lock(&var_mutex);
		numOfThreads--;
		pthread_mutex_unlock(&var_mutex);
		return NULL;
	}
	
	char* hostname = malloc(SBUFSIZE*sizeof(char));
	
	while(fscanf(inputFile, INPUTFS, hostname) > 0){
		char* name = strdup(hostname); //copies hostname to put in queue
		
		//if queue is full, wait
		while(queue_is_full(&buffer)){
			usleep(rand() % 100);
		}
		
		//push hostname onto queue
		pthread_mutex_lock(&buffer_mutex);				
		if(queue_push(&buffer, (void*) name) == QUEUE_FAILURE){
			fprintf(stderr, "Error: queue_push failed.\n");
		}
		pthread_mutex_unlock(&buffer_mutex);		
	}
	
	pthread_mutex_lock(&var_mutex);
	numOfThreads--;
	pthread_mutex_unlock(&var_mutex);
	
	free(hostname); //deallocate memory
	fclose(inputFile); //close input file
    return NULL;
}

void* resolveDNS(void* output){
	FILE* outputFile = fopen((char*) output, "a");
	if(!outputFile){
		fprintf(stderr, "Error Opening Output File.\n");
		return NULL;
	}
	char* hostname;
	char ipString[INET6_ADDRSTRLEN];
	
	while(1){
		pthread_mutex_lock(&buffer_mutex);
		hostname = queue_pop(&buffer); //mutual exclusion on queue
		pthread_mutex_unlock(&buffer_mutex);
		
		if(hostname != NULL){
			if(dnslookup(hostname, ipString, sizeof(ipString)) == UTIL_FAILURE){
				printf("DNSLookup Error: %s\n", hostname);
				strncpy(ipString, "", sizeof(ipString));
			}
			pthread_mutex_lock(&output_mutex);
			fprintf(outputFile, "%s,%s\n", hostname, ipString); //print hostname and ip to output file
			pthread_mutex_unlock(&output_mutex);
			
			free(hostname); //deallocate memory
		}
		else if(numOfThreads == 0){
			break;
		}
	}
	
	free(hostname); //deallocate memory
	fclose(outputFile); //close output file
	return NULL;
}

int main(int argc, char* argv[]){
	// args b/t call to multi-lookup and output file are the files to open
	int numOfFiles = argc - 2;
	
	if(argc < MINARGS){
		fprintf(stderr, "Not enough arguments (need at least 3, found %d).\n", (argc-1));
		return EXIT_FAILURE;
	}
	
	queue_init(&buffer, 50); //initialize queue, 50 = size
	numOfThreads = 0;
	
	//initialize the locks
	int buffmutex = pthread_mutex_init(&buffer_mutex, NULL);
	int outmutex = pthread_mutex_init(&output_mutex, NULL);
	int varmutex = pthread_mutex_init(&var_mutex, NULL);
	
	if(buffmutex || outmutex || varmutex){
		fprintf(stderr, "Pthread mutex did not initalize correctly.\n");
	}
	
	int numThreads = sysconf(_SC_NPROCESSORS_ONLN); //get number of CPU cores
	//must be at least 2 threads
	if(numThreads < NUM_THREADS){
		numThreads = NUM_THREADS;
	}
	
	pthread_t requestThreads[numOfFiles]; //request threads = number of input files
	pthread_t resolveThreads[numThreads]; //resolve threads = number of cores
	
	int i;
	void* state;
	
	//create requester threads
	for(i = 0; i < numOfFiles; i++){
		pthread_create(&requestThreads[i], NULL, requestDNS, (void *)argv[i+1]);
	}
	//create resolver threads
	for(i = 0; i < numThreads; i++){
		pthread_create(&requestThreads[i], NULL, resolveDNS, (void *)argv[argc-1]);
	}
	//wait for all requester threads to finish
	for(i = 0; i < numOfFiles; i++){
		pthread_join(requestThreads[i], &state);
	}
	//wait for all resolver threads to finish
	for(i = 0; i < numThreads; i++){
		pthread_join(resolveThreads[i], &state);
	}
	
	queue_cleanup(&buffer);
	//destroy locks
	pthread_mutex_destroy(&buffer_mutex);
	pthread_mutex_destroy(&output_mutex);
	pthread_mutex_destroy(&var_mutex);
	pthread_exit(NULL);
	
	return 0;
}
