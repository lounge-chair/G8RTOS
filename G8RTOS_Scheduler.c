/*
 * G8RTOS_Scheduler.c
 */

/*********************************************** Dependencies and Externs *************************************************************/
#include <stdint.h>
#include "msp.h"
#include "BSP.h"
#include "G8RTOS_Scheduler.h"
#include "G8RTOS_Structures.h"
#include "G8RTOS_CriticalSection.h"

#define SHPR3 (*((volatile unsigned int *)(0xe000ed20)))
#define PendSV_Priority (0xFF << 16)
#define SysTick_Priority (0xFF << 24)

/*
 * G8RTOS_Start exists in asm
 */
extern void G8RTOS_Start();

/*
 * System Core Clock From system_msp432p401r.c
 */
extern uint32_t SystemCoreClock;

/*
 * Pointer to the currently running Thread Control Block
 */
extern tcb_t * CurrentlyRunningThread;

/*********************************************** Dependencies and Externs *************************************************************/


/*********************************************** Defines ******************************************************************************/

/* Status Register with the Thumb-bit Set */
#define THUMBBIT 0x01000000

/*********************************************** Defines ******************************************************************************/


/*********************************************** Data Structures Used *****************************************************************/

/* Thread Control Blocks
 *	- An array of thread control blocks to hold pertinent information for each thread
 */
static tcb_t threadControlBlocks[MAX_THREADS];

/* Periodic Thread Control Blocks
 *  - An array of periodic thread control blocks to hold pertinent information for each thread
 */
static ptcb_t periodicThreadControlBlocks[MAX_PERIODIC_THREADS];


/* Thread Stacks
 *	- An array of arrays that will act as individual stacks for each thread
 */
static int32_t threadStacks[MAX_THREADS][STACKSIZE];

/*********************************************** Data Structures Used *****************************************************************/


/*********************************************** Private Variables ********************************************************************/

/*
 * Current Number of Threads currently in the scheduler
 */
static uint32_t NumberOfThreads;

/*
 * Current Number of Periodic Threads currently in the scheduler
 */
static uint32_t NumberOfPeriodicThreads;

/*
 * Increments to set unique ID for each thread
 */
static uint16_t IDCounter;

/*********************************************** Private Variables ********************************************************************/


/*********************************************** Private Functions ********************************************************************/

/*
 * Initializes the Systick and Systick Interrupt
 * The Systick interrupt will be responsible for starting a context switch between threads
 * Param "numCycles": Number of cycles for each systick interrupt
 */
static void InitSysTick(void)
{
    SysTick_Config(ClockSys_GetSysFreq()/1000);     // set time quantum to 1ms
    SysTick_enableInterrupt();
}

/*
 * Chooses the next thread to run.
 * Scheduling Algorithm:
 * 	- Priority Round Robin: Choose the next running thread by selecting the next thread with the lowest # priority (highest prio)
 * 	- If scheduler finds that next running thread is asleep or blocked, tries next thread
 */
void G8RTOS_Scheduler()
{
    uint8_t currentMaxPriority = 255;

    tcb_t * tempNextThread = CurrentlyRunningThread->next;

    for(int i = 0; i < NumberOfThreads; i++)
    {
        if(!(tempNextThread->asleep) && !(tempNextThread->blocked))
        {
            if(tempNextThread->priority < currentMaxPriority)
            {
                CurrentlyRunningThread = tempNextThread;
                currentMaxPriority = tempNextThread->priority;
            }
        }
        tempNextThread = tempNextThread->next;
    }
}

/*
 * SysTick Handler
 */
void SysTick_Handler()
{
    SystemTime++;

    // PERIODIC THREADS - run periodic threads at their execute time
    if (NumberOfPeriodicThreads > 0)
    {
        ptcb_t * pPtr = &periodicThreadControlBlocks[0];
        for (int i = 0 ; i < NumberOfPeriodicThreads ; i++)
        {
            if (SystemTime >= pPtr->executeTime)
            {
                pPtr->executeTime = SystemTime + pPtr->period;      // new execute time is 1 period from current time
                pPtr->handler();                                    // execute the code at the handler
            }
            pPtr = pPtr->next;
        }
    }

    // SLEEPING THREADS - check to see if the NEXT thread is asleep and if it is time to wake it up (sleep count = system time)
    tcb_t * nextThread = CurrentlyRunningThread->next;
    for (int i = 0 ; i < MAX_THREADS ; i++)
    {
        if ((nextThread->asleep) && (nextThread->sleepCount <= SystemTime))
        {
            nextThread->asleep = false;     // wake up thread
        }
        nextThread = nextThread->next;
    }

    // Trigger context switch
    G8RTOS_Yield();

}

void G8RTOS_Yield()
{
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

/*********************************************** Private Functions ********************************************************************/


/*********************************************** Public Variables *********************************************************************/

/* Holds the current time for the whole System */
uint32_t SystemTime;

/*********************************************** Public Variables *********************************************************************/


/*********************************************** Public Functions *********************************************************************/

/*
 * Sets variables to an initial state (system time and number of threads)
 * Enables board for highest speed clock and disables watchdog
 */
void G8RTOS_Init()
{
    // Set vars to initial state
    SystemTime = 0;         // Set system time to initial value of 0
    NumberOfThreads = 0;    // Set number of threads to initial value of 0
    NumberOfPeriodicThreads = 0;
    IDCounter = 0;
    CurrentlyRunningThread = &threadControlBlocks[0];
    // Create new vector table in SRAM
    uint32_t newVTORTable = 0x20000000;
    memcpy((uint32_t *)newVTORTable, (uint32_t *)SCB->VTOR, 57*4);  // 57 interrupt vectors to copy
    SCB->VTOR = newVTORTable;
    BSP_InitBoard();        // Initialize all hardware on the board
}

/*
 * Starts G8RTOS Scheduler
 * 	- Initializes the Systick
 * 	- Sets Context to first thread
 * Returns: Error Code for starting scheduler. This will only return if the scheduler fails
 */
int G8RTOS_Launch()
{
    uint8_t currentMaxPriority = 255;

    tcb_t * tempNextThread = &threadControlBlocks[0];

    for(int i = 0; i < NumberOfThreads; i++)                // sets CurrentlyRunningThread to highest priority thread in list
    {
        if(tempNextThread->priority < currentMaxPriority)
        {
            CurrentlyRunningThread = tempNextThread;
            currentMaxPriority = tempNextThread->priority;
        }
    }

    InitSysTick();                                              // Initialize SysTick
    SHPR3 |= SysTick_Priority | PendSV_Priority | 0x0000;       // Set PendSV to high priority (low #)
    G8RTOS_Start();
    return 1;   //RETURN ERROR: should not return from Start function
}


/*
 * Adds threads to G8RTOS Scheduler
 * 	- Checks if there are still available threads to insert to scheduler
 * 	- Initializes the thread control block for the provided thread
 * 	- Initializes the stack for the provided thread to hold a "fake context"
 * 	- Sets stack tcb stack pointer to top of thread stack
 * 	- Sets up the next and previous tcb pointers in a round robin fashion
 * Param "threadToAdd": Void-Void Function to add as preemptable main thread
 * Returns: Error code for adding threads
 */
sched_ErrCode_t G8RTOS_AddThread(void (*threadToAdd)(void), uint8_t priority, char* threadname)
{
    uint32_t savedmask = StartCriticalSection(); // disable interrupts (start critical section)

    uint32_t tcbToInitialize = MAX_THREADS;
    for(int i = 0; i < MAX_THREADS; i++)
    {
        if(!threadControlBlocks[i].alive)
        {
            tcbToInitialize = i;
            break;
        }
    }

    if (tcbToInitialize < MAX_THREADS)  // Checks if there are still available threads to insert to scheduler
    {
        setInitialStack(threadToAdd, tcbToInitialize);                                               // initialize fake context for new thread
        threadControlBlocks[tcbToInitialize].sp = &threadStacks[tcbToInitialize][STACKSIZE-16];   // points to stack where core regs will be popped from in PendSV
        if (NumberOfThreads == 0)
        {
            threadControlBlocks[0].next = &threadControlBlocks[0];              // Next is set to current TCB
            threadControlBlocks[0].prev = &threadControlBlocks[0];              // Prev is set to current TCB
        }
        else
        {
            threadControlBlocks[tcbToInitialize].prev = CurrentlyRunningThread;
            threadControlBlocks[tcbToInitialize].next = CurrentlyRunningThread->next;
            threadControlBlocks[tcbToInitialize].next->prev = &threadControlBlocks[tcbToInitialize];
            CurrentlyRunningThread->next = &threadControlBlocks[tcbToInitialize];
        }

        threadControlBlocks[tcbToInitialize].asleep = false;
        threadControlBlocks[tcbToInitialize].priority = priority;
        threadControlBlocks[tcbToInitialize].threadID = ((IDCounter++) << 16 | tcbToInitialize);
        threadControlBlocks[tcbToInitialize].alive = true;
        strcpy(threadControlBlocks[tcbToInitialize].threadName, threadname);

        NumberOfThreads++;
        EndCriticalSection(savedmask);                  // enable interrupts (end critical section)
        return NO_ERROR;
    }
    else
    {
        EndCriticalSection(savedmask);                  // enable interrupts (end critical section)
        return THREADS_INCORRECTLY_ALIVE;       // RETURN ERROR (cannot have greater than MAX_THREADS threads)
    }
}

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
int G8RTOS_AddPeriodicThread(void (*threadToAdd)(void), uint32_t period)
{
    uint32_t savedmask = StartCriticalSection(); // disable interrupts (start critical section)
    if (NumberOfPeriodicThreads < MAX_PERIODIC_THREADS)
    {
        // initialize periodic tcb for new periodic thread
        periodicThreadControlBlocks[NumberOfPeriodicThreads].handler = threadToAdd;
        periodicThreadControlBlocks[NumberOfPeriodicThreads].period = period;
        periodicThreadControlBlocks[NumberOfPeriodicThreads].currentTime = SystemTime;

        if (NumberOfPeriodicThreads == 0)
        {
            periodicThreadControlBlocks[NumberOfPeriodicThreads].executeTime = SystemTime + period;
            periodicThreadControlBlocks[0].next = &periodicThreadControlBlocks[0];
            periodicThreadControlBlocks[0].prev = &periodicThreadControlBlocks[0];
        }
        else if (NumberOfPeriodicThreads > 0)
        {
            periodicThreadControlBlocks[NumberOfPeriodicThreads].executeTime = SystemTime + period + 1 + NumberOfPeriodicThreads;
            periodicThreadControlBlocks[0].prev = &periodicThreadControlBlocks[NumberOfPeriodicThreads];
            periodicThreadControlBlocks[NumberOfPeriodicThreads-1].next = &periodicThreadControlBlocks[NumberOfPeriodicThreads];
            periodicThreadControlBlocks[NumberOfPeriodicThreads].prev = &periodicThreadControlBlocks[NumberOfPeriodicThreads-1];
            periodicThreadControlBlocks[NumberOfPeriodicThreads].next = &periodicThreadControlBlocks[0];
        }
        else {
            EndCriticalSection(savedmask);                  // enable interrupts (end critical section)
            return 1;   // RETURN ERROR (impossible negative NumberOfPeriodicThreads value)
        }

        NumberOfPeriodicThreads++;
        EndCriticalSection(savedmask);                  // enable interrupts (end critical section)
        return 0;
    }
    else
    {
        EndCriticalSection(savedmask);                  // enable interrupts (end critical section)
        return 1;       // RETURN ERROR (cannot have greater than MAX_PERIODIC_THREADS periodic threads)
    }
}

/*
 * Adds aperiodic event to G8RTOS Scheduler
 */
sched_ErrCode_t G8RTOS_AddAPeriodicEvent(void (*AthreadToAdd) (void), uint8_t priority, IRQn_Type IRQn)
{
    uint32_t savedmask = StartCriticalSection();
    if( IRQn < PSS_IRQn || IRQn > PORT6_IRQn)
    {
        EndCriticalSection(savedmask);
        return IRQn_INVALID;
    }
    if(!(priority < OSINT_PRIORITY))
    {
        EndCriticalSection(savedmask);
        return HWI_PRIORITY_INVALID;
    }
    __NVIC_SetVector(IRQn, AthreadToAdd);
    __NVIC_SetPriority(IRQn, priority);
    __NVIC_EnableIRQ(IRQn);
    P4->IFG &= ~BIT0;
    EndCriticalSection(savedmask);
    return NO_ERROR;
}

/* - Sets dummy values for the stacks of each thread
 * - R0-R3, R12, PC, LR, PSR get auto pushed onto stack (does not push SP)
 * - sets PC to thread address
 */
void setInitialStack(void (*threadToAdd)(void), uint32_t tcbToInitialize)
{
    threadStacks[tcbToInitialize][STACKSIZE-1]  = THUMBBIT;                      // set thumb bit in PSR; PSR is auto pushed to STACKSIZE-1
    threadStacks[tcbToInitialize][STACKSIZE-2]  = (int32_t)(threadToAdd);        // set PC to new thread address
    threadStacks[tcbToInitialize][STACKSIZE-3]  = 0x14141414;                    // R14/LR
    threadStacks[tcbToInitialize][STACKSIZE-4]  = 0x12121212;                    // R12
    threadStacks[tcbToInitialize][STACKSIZE-5]  = 0x03030303;                    // R3
    threadStacks[tcbToInitialize][STACKSIZE-6]  = 0x02020202;                    // R2
    threadStacks[tcbToInitialize][STACKSIZE-7]  = 0x01010101;                    // R1
    threadStacks[tcbToInitialize][STACKSIZE-8]  = 0x00000000;                    // R0
    threadStacks[tcbToInitialize][STACKSIZE-9]  = 0x0B0B0B0B;                    // R11
    threadStacks[tcbToInitialize][STACKSIZE-10] = 0x0A0A0A0A;                    // R10
    threadStacks[tcbToInitialize][STACKSIZE-11] = 0x09090909;                    // R9
    threadStacks[tcbToInitialize][STACKSIZE-12] = 0x08080808;                    // R8
    threadStacks[tcbToInitialize][STACKSIZE-13] = 0x07070707;                    // R7
    threadStacks[tcbToInitialize][STACKSIZE-14] = 0x06060606;                    // R6
    threadStacks[tcbToInitialize][STACKSIZE-15] = 0x05050505;                    // R5
    threadStacks[tcbToInitialize][STACKSIZE-16] = 0x04040404;                    // R4
}

/* - Put current thread to sleep
 */
void G8RTOS_Sleep(uint32_t duration)
{
    CurrentlyRunningThread->sleepCount = duration + SystemTime;
    CurrentlyRunningThread->asleep = true;
    G8RTOS_Yield();
}

threadId_t G8RTOS_GetThreadID()
{
    return CurrentlyRunningThread->threadID;
}

sched_ErrCode_t G8RTOS_KillThread(threadId_t threadId)
{
    uint32_t savedmask = StartCriticalSection();
    if(NumberOfThreads == 0){
        EndCriticalSection(savedmask);
        return CANNOT_KILL_LAST_THREAD;
    }
    uint32_t tcbToKill = MAX_THREADS;
    for(int i = 0; i < MAX_THREADS; i++)
    {
        if(threadControlBlocks[i].threadID == threadId)
        {
            tcbToKill = i;
            break;
        }
    }
    if(tcbToKill < MAX_THREADS)
    {
        threadControlBlocks[tcbToKill].alive = false;
        threadControlBlocks[tcbToKill].next->prev = threadControlBlocks[tcbToKill].prev;
        threadControlBlocks[tcbToKill].prev->next = threadControlBlocks[tcbToKill].next;
        NumberOfThreads--;
        EndCriticalSection(savedmask);
        if(threadControlBlocks[tcbToKill].threadID == CurrentlyRunningThread->threadID){
            G8RTOS_Yield();
        }
    }else
    {
        EndCriticalSection(savedmask);
        return THREAD_DOES_NOT_EXIST;
    }
    return NO_ERROR;
}

sched_ErrCode_t G8RTOS_KillSelf(){
    return G8RTOS_KillThread(CurrentlyRunningThread->threadID);
}

sched_ErrCode_t G8RTOS_KillAllOthers()
{
    tcb_t *ptr = CurrentlyRunningThread;
    for (int i = 0; i < NumberOfThreads; i++)
    {
        if(ptr->threadID != CurrentlyRunningThread->threadID)
        {
            ptr = ptr->next;
            G8RTOS_KillThread(ptr->threadID);
        }
    }
}
/*********************************************** Public Functions *********************************************************************/
