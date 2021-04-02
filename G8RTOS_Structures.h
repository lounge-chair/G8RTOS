/*
 * G8RTOS_Structure.h
 *
 *      Author: Dan
 */

#ifndef G8RTOS_STRUCTURES_H_
#define G8RTOS_STRUCTURES_H_

#include <G8RTOS.h>
#include "stdbool.h"
#include "G8RTOS_Semaphores.h"
#define MAX_NAME_LENGTH     10

/*********************************************** Data Structure Definitions ***********************************************************/

/*
 *  Thread Control Block:
 *      - Every thread has a Thread Control Block
 *      - The Thread Control Block holds information about the Thread Such as the Stack Pointer, Priority Level, and Blocked Status
 *      - The TCB will only hold the Stack Pointer, next TCB and the previous TCB (for Round Robin Scheduling)
 */
typedef struct tcb_t
{
    int32_t * sp;           // stack pointer for this thread
    struct tcb_t * next;    // pointer to next tcb
    struct tcb_t * prev;    // pointer to previous tcb
    semaphore_t * blocked;  // blocking semaphore
    uint32_t sleepCount;    // how long thread has been asleep
    bool asleep;            // thread waits for certain amnt of time before it enters active state
    uint8_t priority;       // thread priority (lower number = higher priority, to match ARM Cortex-M convention)
    bool alive;
    uint32_t threadID;
    char threadName[MAX_NAME_LENGTH];

} tcb_t;

/*
 * Periodic Thread Control Block
 */
typedef struct ptcb_t
{
    void (*handler)(void);
    uint32_t period;
    uint32_t executeTime;
    uint32_t currentTime;
    struct pEvent_t * next;
    struct pEvent_t * prev;
} ptcb_t;

typedef uint32_t threadId_t;

typedef enum
{
        NO_ERROR                    = 0,
        THREAD_LIMIT_REACHED        = -1,
        NO_THREADS_SCHEDULED        = -2,
        THREADS_INCORRECTLY_ALIVE   = -3,
        THREAD_DOES_NOT_EXIST       = -4,
        CANNOT_KILL_LAST_THREAD     = -5,
        IRQn_INVALID                = -6,
        HWI_PRIORITY_INVALID        = -7
} sched_ErrCode_t;

/*********************************************** Data Structure Definitions ***********************************************************/


/*********************************************** Public Variables *********************************************************************/

tcb_t * CurrentlyRunningThread;

/*********************************************** Public Variables *********************************************************************/

#endif /* G8RTOS_STRUCTURES_H_ */
