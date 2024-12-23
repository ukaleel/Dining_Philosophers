/*
Dining Philosophers Problem
CSCE 420 Operating Systems
Usman Kaleel

*/
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

//define constant PHILOSOPHERS as 7
#define PHILOSOPHERS 7
//define number of times function philosopher runs, per thread, as 20
#define NUMBEROFLOOPS 20

pthread_mutex_t lock; //lock for grabbing forks

pthread_mutex_t writingLock; // lock for writing to the text file

pthread_mutex_t discussLock; //lock for discussing

//a condition variable for discussing
pthread_cond_t discussCondition;

//a condition variable for grabbing forks
pthread_cond_t grabbingCondition;

//variable to count the number of discussing philosophers
int numberOfDiscussingThreads=0;


//an array to represent the forks
int forks[PHILOSOPHERS] = {0};


// number of alive even threads, used in discussions
int evenThreadsAlive = 0;

//function headers
void * thinking(void * tid);
void * eating(void * tid);
void * take_fork(void * tid);
void * discussing(void * tid);
void * put_fork(void * tid);
void * philosophers(void * tid);



// putting forks away function
void * put_fork(void * tid) {
	int status;
	int philosopherID = (intptr_t)tid;
	char command[150];
	int leftFork = philosopherID % PHILOSOPHERS;
	int rightFork = philosopherID-1 ;

    // grabs lock before changing the array
	pthread_mutex_lock(&lock);

	forks[rightFork] = 0;
	forks[leftFork] = 0;

    //broadcasts to anyone waiting to grab a fork as it puts down its forks
	pthread_cond_broadcast(&grabbingCondition);

	sprintf(command, "echo 'Philosopher %d is putting fork %d (left) and fork %d (right)' >> philosophers_output.txt", philosopherID, leftFork, rightFork);

	printf("Philosopher %d is putting fork %d (left) and fork %d (right)\n", philosopherID, leftFork, rightFork);
    
    // grabs writing lock before altering text file
    pthread_mutex_lock(&writingLock);
	status = system(command);

	pthread_mutex_unlock(&lock);
	pthread_mutex_unlock(&writingLock);
}


// take fork function: takes forks while maintaining mutual exclusion
void * take_fork(void * tid) {
	int status;
	int philosopherID = (intptr_t)tid;
	char command[150];
	int leftFork = philosopherID % PHILOSOPHERS;
	int rightFork = philosopherID-1 ;

    //grabs lock before trying to alter array 
	pthread_mutex_lock(&lock);

    // if the forks are taken, wait until they are free
	while(forks[rightFork] || forks[leftFork]) {
		pthread_cond_wait(&grabbingCondition, &lock);
	}

	forks[rightFork] = 1;
	forks[leftFork] = 1;

	sprintf(command, "echo 'Philosopher %d is taking fork %d (left) and fork %d (right)' >> philosophers_output.txt", philosopherID, leftFork, rightFork);
	printf("Philosopher %d is taking fork %d (left) and fork %d (right) \n", philosopherID, leftFork, rightFork);
	
	
	//grabs writing lock prior to altering text file
	pthread_mutex_lock(&writingLock);
	status = system(command);

	pthread_mutex_unlock(&lock);
	pthread_mutex_unlock(&writingLock);
}


//discussing function: shows philosophers are discussing
void * discussing(void * tid) {
	int status;
	int philosopherID = (intptr_t)tid;
	char command[150];

// if there are not enough even forks alive, don't discuss
// more so made for failed attempt at odd implementation
	if (evenThreadsAlive < 3) {
		sprintf(command, "echo 'Philosopher %d cannot join a discussion due to insufficient number of even threads alive\n' >> philosophers_output.txt", philosopherID);
		printf("Philosopher %d cannot join a discussion due to insufficient number of odd threads alive\n", philosopherID);
		pthread_mutex_lock(&writingLock);
    	status = system(command);
    	pthread_mutex_unlock(&writingLock);
		pthread_cond_broadcast(&discussCondition); // in case any threads get stuck somehow
		return 0;
	}


    //grabs discuss lock prior to starting
	pthread_mutex_lock(&discussLock);
	sprintf(command, "echo 'Philosopher %d is joining the discussion with %d others\n' >> philosophers_output.txt", philosopherID, numberOfDiscussingThreads);
	printf("Philosopher %d is joining the discussion with %d others\n", philosopherID, numberOfDiscussingThreads);
	pthread_mutex_lock(&writingLock);
    status = system(command);
    pthread_mutex_unlock(&writingLock);
	numberOfDiscussingThreads+=1;


    //broadcasts if it is equal to 3
	if (numberOfDiscussingThreads == 3) {
		char otherCommand[150];
		sprintf(otherCommand, "echo 'Philosopher %d is ending the discussion' >> philosophers_output.txt", philosopherID );
		printf("Philosopher %d is ending the discussion\n", philosopherID);
		status = system(otherCommand);


		numberOfDiscussingThreads = 0;

		pthread_cond_broadcast(&discussCondition);
	}
	//otherwise, wait for the broadcast
	else {
		if (numberOfDiscussingThreads > 0 && numberOfDiscussingThreads<3) {
			pthread_cond_wait(&discussCondition, &discussLock);
		}

	}


	pthread_mutex_unlock(&discussLock);



}



//thinking function: shows philospher is thinking
void * thinking(void * tid) {
	int status;
	int philosopherID = (intptr_t)tid;
//This function prints the threadb  s identifier and that it's thinking.
	printf("Philosopher thread %d is thinking\n", philosopherID);

//create a bash command to add output to file
	char command[100];
	sprintf(command, "echo 'Philosopher %d thinking\n' >> philosophers_output.txt", philosopherID);
	
	//grabs writing lock prior to changing text file
	pthread_mutex_lock(&writingLock);
	status = system(command);
	pthread_mutex_unlock(&writingLock);

}

// philosopher eating function
void * eating(void * tid) {
	int status;
	int philosopherID = (intptr_t)tid;
//This function prints the threadb  s identifier and that it's eating.
	printf("Philosopher thread %d is eating\n", philosopherID);

//create a bash command to add output to file
	char command[100];
	sprintf(command, "echo 'Philosopher %d eating\n' >> philosophers_output.txt", philosopherID);

//grabs writing lock prior to changing text file
	pthread_mutex_lock(&writingLock);
	status = system(command);
	pthread_mutex_unlock(&writingLock);


}


void * philosopher(void * philosopherID) {
	int intPhiloID = (intptr_t)philosopherID;
	if (intPhiloID % 2 == 0) { // increment if even
		evenThreadsAlive++;
	}

	for(int i = 0; i < NUMBEROFLOOPS; i++) {
		thinking(philosopherID);
		take_fork(philosopherID);
		eating(philosopherID);
		if (intPhiloID % 2 == 0) { // if the id is even, discuss
			discussing(philosopherID);
		}

		put_fork(philosopherID);
	}
	if (intPhiloID % 2 == 0) { //decrement if even as it exits
		evenThreadsAlive--;
	}
/// DEBUGGING STATEMENT
//	printf("Philosopher thread %d has completed. \n",intPhiloID);
//	printf("There are %d even threads alive\n",evenThreadsAlive);

	//example of making a thread leave the program
	pthread_exit(NULL);
}

int main(int argc, char * argv[])
{
	/* The main program creates 7 threads and then exits. */
	pthread_t threads[PHILOSOPHERS];
	int status, i;

	//create a mutex
	if (pthread_mutex_init(&lock, NULL) != 0)
	{
		printf("\n mutex init failed\n");
		return 1;
	}
	//count through 7 threads
	for(i=1; i < PHILOSOPHERS+1; i++) {
		//run main thread
		printf("Main here. Creating philosopher thread %d\n", i);
		//create 7 threads
		status = pthread_create(&threads[i], NULL, philosopher, (void *)(intptr_t)i);
		//check for errors with creating threads
		if (status != 0) {
			printf("Oops. pthread create returned error code %d\n", status);
			exit(-1);
		}

	}
	//have the main thread wait for all the threads created
	for (int j = 1; j < PHILOSOPHERS+1; j++)
	{
		pthread_join (threads[j], NULL);
	}
	//delete the mutex
	pthread_mutex_destroy(&lock);
	exit(0);
}
