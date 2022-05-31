// ICS53 - Assignment 5 Exercises

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <pthread.h>

/* This example demonstrates two different threads that try to modify the value of the global_variable at the same time.
 * The first thread increases the value of the global_variable by 1 three times.
 * The second thread decreases the value of the global_variable by 1 three times.
 * */

int global_variable = 5;
pthread_mutex_t lock;

void *myThreadExample(void *vargp)
{
    /* pthread_mutex_lock will lock the entire block of code below until the executing thread reaches pthread_mutex_unlock */
//    int lock_success = pthread_mutex_lock(&lock);
//    printf("lock_success = %i\n", lock_success);

    /* pthread_mutex_trylock will make it optional for other threads whether or not wait for the lock to be released */
    int lock_status = pthread_mutex_trylock(&lock);
    printf("lock_status = %i\n", lock_status);
    while (lock_status !=0){  // Trapping the locked thread in a loop until the lock is released.
        usleep(100000);  // 0.1s
        lock_status = pthread_mutex_trylock(&lock);
    }


    printf("My arg is: %i\n", vargp);
    pthread_t thread_id = pthread_self();
    printf("My thread_id = %i\n", thread_id);

    int i = (int)vargp;
    if (i == 3){  // code for the increasing thread.

        printf("global_variable now = %i\n", global_variable);
        global_variable = global_variable + 1;
        printf("global_variable after increase = %i\n", global_variable);
        global_variable = global_variable + 1;
        printf("global_variable after increase = %i\n", global_variable);
        usleep(2000000);
        global_variable = global_variable + 1;
        printf("global_variable after increase = %i\n", global_variable);

    }
    if (i == 2){  // code for the decreasing thread.

        printf("global_variable now = %i\n", global_variable);
        global_variable = global_variable - 1;
        printf("global_variable after decrease = %i\n", global_variable);
        global_variable = global_variable - 1;
        printf("global_variable after decrease = %i\n", global_variable);
        global_variable = global_variable - 1;
        printf("global_variable after decrease = %i\n", global_variable);
    }

    // Release the lock for the next thread.
    pthread_mutex_unlock(&lock);
    return NULL;
}

int main()
{

    pthread_t thread_id1;
    pthread_t thread_id2;
    pthread_mutex_init(&lock, NULL);
    int i1 = 3;
    int i2 = 2;
    printf("Start Threads\n");
    pthread_create(&thread_id1, NULL, myThreadExample, (void *)i1);
    pthread_create(&thread_id2, NULL, myThreadExample, (void *)i2);
    pthread_join(thread_id1, NULL);
    pthread_join(thread_id2, NULL);
    pthread_mutex_destroy(&lock);
    printf("End Threads\n");
    exit(0);
}

