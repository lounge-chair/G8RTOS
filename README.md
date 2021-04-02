# G8RTOS
Implementation of G8RTOS, a real-time operating system featuring priority scheduling, background/periodic/aperiodic threads, inter-process communication, semaphores and more, designed for the MSP432.

*Sample Initalization:*

```
void main(void)
{
    WDT_A_clearTimer();                                   // disable watchdog
    WDT_A_holdTimer();

    G8RTOS_Init();                                        // initialize OS

    G8RTOS_InitSemaphore(&sampleMutex, 1);                // initialize semaphore for mutual exclusion

    G8RTOS_InitFIFO(SAMPLE_FIFO);                         // initialize the FIFO 
    
    G8RTOS_AddThread(&thread1,   1,  "thread1");          // add background threads to scheduler
    G8RTOS_AddThread(&thread2,   1,  "thread2");
    G8RTOS_AddThread(&thread3,   5,  "thread3");
    G8RTOS_AddPeriodicThread(&pthread1, 100);             // add periodic thread to scheduler
    G8RTOS_AddAPeriodicEvent(&sampleISR, 0, PORT4_IRQn);  // add aperiodic event to scheduler

    G8RTOS_Launch();                                      // launch OS

    while(1);                                             // never reached
}
```
