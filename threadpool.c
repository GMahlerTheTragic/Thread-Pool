#include "threadpool.h"
#include "array.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <limits.h>
#ifndef __STDC_NO_ATOMICS__
#include <stdatomic.h>
#endif

size_t pool_size;											// Keeps track of pool size
pthread_t *tids;											// Stores thread ids
Future **task_stack;										// Stores the created Futures in a stack. This LIFO logic should be important since we are using recursion right?
sem_t events;												// Tracks writes to the Task Stack and also tracks finshed Futures
pthread_mutex_t mutexTaskStack = PTHREAD_MUTEX_INITIALIZER; // Mutex to make sure Task Stack is manipulated correclty

/* Run a task as outlined by the Thunk function created by the macro */
void executeTask(Future *task)
{
	task->fn(task);
	atomic_store(&task->terminated, 1); // set terminated to 1 atomically signaling finished execution
	sem_post(&events);					// Increase event count by one
}

void *startThread(void *args)
{
	// Run until terminated
	while (1)
	{
		// Wait for a potential non empty task stack
		sem_wait(&events);
		// Check if the event was indeed a non empty task stack or just some finished task
		pthread_mutex_lock(&mutexTaskStack);
		if (arrayLen(task_stack) > 0)
		{
			// if so pop the task and run
			Future *task;
			task = arrayPop(task_stack);
			pthread_mutex_unlock(&mutexTaskStack);

			executeTask(task);
		}
		else
		{
			pthread_mutex_unlock(&mutexTaskStack);
			// if not put the event back into storage. It was just some finshed future
			sem_post(&events);
		}
	}
}

int tpInit(size_t size)
{
	// Manully increase stack size for threads
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	size_t stacksize = 2048UL * 1024UL * 1024UL;
	pthread_attr_setstacksize(&attr, stacksize);

	// Initialize task stack and thread id array
	arrayInit(task_stack);
	tids = malloc(size * sizeof(pthread_t));

	// Initialize event semaphore
	sem_init(&events, 0, 0);

	// Spawn Threads
	for (size_t i = 0; i < size; i++)
	{
		if (pthread_create(&tids[i], &attr, &startThread, NULL) != 0)
		{
			perror("Error creating thread");
			// Return with non zero error code
			return 1;
		};
	}

	// Keep track of pool size
	pool_size = size;

	// Tell us about the threads
	printf("Initialized Thread pool with %ld threads \n", size);
	for (size_t i = 0; i < size; i++)
		printf("Thread # %ld: ID [%ld]  \n", i, tids[i]);

	return 0;
}

// Clean up
void tpRelease(void)
{

	// Cancel threads
	for (size_t i = 0; i < pool_size; i++)
	{
		pthread_cancel(tids[i]);
		printf("Thread # %ld: ID [%ld] cancelled \n", i, tids[i]);
	}

	// Free event semaphore
	sem_destroy(&events);

	// Free task stak
	arrayRelease(task_stack);

	// Free array of thread ids
	free(tids);

	printf("thread pool released \n");
}

// Initiate a new future
void tpAsync(Future *future)
{
	// Initialize atomic int to keep track of termination state
	atomic_init(&future->terminated, 0);
	// Push onto task stack
	pthread_mutex_lock(&mutexTaskStack);
	arrayPush(task_stack) = future;

	// Signify that there was an event (In this case a stack push)
	sem_post(&events);
	pthread_mutex_unlock(&mutexTaskStack);
}

void tpAwait(Future *future)
{
	// Keep in waiting loop until returned (i.e. until Result from Future was obtained)
	while (1)
	{

		// Sleep until there was at least a potential event
		sem_wait(&events);

		// Check if it was our task termination
		if (atomic_load(&future->terminated) == 1)
		{
			// If so, return
			return;
		}

		// If not use the time: The semaphore permitted us entry so it must have been...

		pthread_mutex_lock(&mutexTaskStack);
		// A non empty task stack - in which case we take over this task
		if (arrayLen(task_stack) > 0)
		{
			Future *task;
			task = arrayPop(task_stack);
			pthread_mutex_unlock(&mutexTaskStack);

			executeTask(task);
		}
		else
		{
			pthread_mutex_unlock(&mutexTaskStack);
			// Or a finished future that we were not waiting for
			// In this case we have to increase the event count by one again
			// (We were allowd in but the event was not for us)
			sem_post(&events);
		}
	}
}