/*
 * G8RTOS_Scheduler.h
 */

#ifndef G8RTOS_SCHEDULER_H_
#define G8RTOS_SCHEDULER_H_

#include <stdint.h>
#include "msp.h"
#include "BSP.h"
#include "G8RTOS_Structures.h"

/*********************************************** Sizes and Limits *********************************************************************/
#define MAX_THREADS 26
#define MAX_PERIODIC_THREADS 6
#define STACKSIZE 512
#define OSINT_PRIORITY 7
/*********************************************** Sizes and Limits *********************************************************************/


/*********************************************** Public Variables *********************************************************************/

/* Holds the current time for the whole System */
extern uint32_t SystemTime;

/*********************************************** Public Variables *********************************************************************/


/*********************************************** Public Functions *********************************************************************/

/*
 * Initializes variables and hardware for G8RTOS usage
 */
void G8RTOS_Init();

/*
 * Starts G8RTOS Scheduler
 * 	- Initializes Systick Timer
 * 	- Sets Context to first thread
 * Returns: Error Code for starting scheduler. This will only return if the scheduler fails
 */
int32_t G8RTOS_Launch();

/*
 * Adds threads to G8RTOS Scheduler
 * 	- Checks if there are stil available threads to insert to scheduler
 * 	- Initializes the thread control block for the provided thread
 * 	- Initializes the stack for the provided thread
 * 	- Sets up the next and previous tcb pointers in a round robin fashion
 * Param "threadToAdd": Void-Void Function to add as preemptable main thread
 * Returns: Error code for adding threads
 */
sched_ErrCode_t G8RTOS_AddThread(void (*threadToAdd)(void), uint8_t priority, char* threadName);

/*
 * Adds periodic threads to G8RTOS Scheduler
 *  - Checks if there are still available threads to insert to scheduler
 *  - Initializes the thread control block for the provided thread
 *  - Initializes the stack for the provided thread to hold a "fake context"
 *  - Sets stack tcb stack pointer to top of thread stack
 *  - Sets up the next and previous tcb pointers in a round robin fashion
 * Param "threadToAdd": Void-Void Function to add as preemptable main thread
 * Returns: Error code for adding threads
 */
int G8RTOS_AddPeriodicThread(void (*threadToAdd)(void), uint32_t period);


/*
 * Adds aperiodic event to G8RTOS Scheduler
 */
sched_ErrCode_t G8RTOS_AddAPeriodicEvent(void (*AthreadToAdd) (void), uint8_t priority, IRQn_Type IRQn);

/*
 * Puts the currently running thread to sleep for <duration> milliseconds
 */
void G8RTOS_Sleep(uint32_t duration);

/* - Sets dummy values for the stacks of each thread
 * - R0-R3, R12, PC, LR, PSR get auto pushed onto stack (does not push SP)
 * - sets PC to thread address
 */
void setInitialStack(void (*threadToAdd)(void), uint32_t tcbToInitialize);

/*
 * Triggers a context switch by setting the PendSV flag
 */
void G8RTOS_Yield();

/*
 * Kills a specific thread, given it's threadID
 */
sched_ErrCode_t G8RTOS_KillThread(threadId_t threadId);

/*
 * Kills thread that calls function
 */
sched_ErrCode_t G8RTOS_KillSelf();

/*
 * Kills all other threads BESIDES the one calling this function
 */
sched_ErrCode_t G8RTOS_KillAllOthers();
/*********************************************** Public Functions *********************************************************************/

#endif /* G8RTOS_SCHEDULER_H_ */
