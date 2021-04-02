/*
 * G8RTOS_Semaphores.c
 */

/*********************************************** Dependencies and Externs *************************************************************/

#include <stdint.h>
#include "msp.h"
#include <G8RTOS/G8RTOS_Semaphores.h>
#include <G8RTOS/G8RTOS_CriticalSection.h>
#include <G8RTOS/G8RTOS_Scheduler.h>
#include <G8RTOS/G8RTOS_Structures.h>


/*********************************************** Dependencies and Externs *************************************************************/


/*********************************************** Public Functions *********************************************************************/

/*
 * Initializes a semaphore to a given value
 * Param "s": Pointer to semaphore
 * Param "value": Value to initialize semaphore to
 * THIS IS A CRITICAL SECTION
 */
void G8RTOS_InitSemaphore(semaphore_t *s, int32_t value)
{
    uint32_t savedmask = StartCriticalSection();             // disable interrupts (end critical section)
    *s=value;
    EndCriticalSection(savedmask);                  // enable interrupts
}

/*
 * Waits for a semaphore to be available (value greater than 0)
 * 	- Decrements semaphore when available
 * 	- Spinlocks to wait for semaphore
 * Param "s": Pointer to semaphore to wait on
 * THIS IS A CRITICAL SECTION
 */
void G8RTOS_AcquireSemaphore(semaphore_t *s)
{
    uint32_t savedmask = StartCriticalSection();  // disable interrupts

    (*s)--;

    if ((*s) < 0)
    {
        CurrentlyRunningThread->blocked = s;

        EndCriticalSection(savedmask);            // enable interrupts

        G8RTOS_Yield();                         // triggers context switch to let other thread go instead
    }

    EndCriticalSection(savedmask);
}

/*
 * Signals the completion of the usage of a semaphore
 * 	- Increments the semaphore value by 1
 * Param "s": Pointer to semaphore to be signalled
 * THIS IS A CRITICAL SECTION
 */
void G8RTOS_ReleaseSemaphore(semaphore_t *s)
{
	uint32_t savedmask = StartCriticalSection();

	(*s)++;     // set the semaphore - resource

	if ((*s) <= 0)
	{
	    tcb_t *pt = CurrentlyRunningThread->next;
	    while(pt->blocked != s)
	    {
	        pt = pt->next;
	    }

	    pt->blocked = 0;
	}

	EndCriticalSection(savedmask);
}

/*********************************************** Public Functions *********************************************************************/
