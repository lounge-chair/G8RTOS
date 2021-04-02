/*
 * G8RTOS_IPC.c
 */

/***************************************************** Includes ***********************************************************************/

#include <G8RTOS.h>
#include <G8RTOS_IPC.h>
#include "G8RTOS_Structures.h"

/***************************************************** Includes ***********************************************************************/

/*************************************************** Defines Used *********************************************************************/

#define MAX_FIFOS 4
#define MAX_FIFO_SIZE 16

/*************************************************** Defines Used *********************************************************************/

/*********************************************** Data Structures Used *****************************************************************/

/*
 * Declaration of the Array of FIFOs
 */
static struct FIFO_t fifoArray[MAX_FIFOS];

/*********************************************** Data Structures Used *****************************************************************/

/*********************************************** Public Functions *********************************************************************/

int G8RTOS_InitFIFO(uint32_t index)
{
    if (index < MAX_FIFOS)
    {
        fifoArray[index].head = &fifoArray[index].buffer[0];
        fifoArray[index].tail = &fifoArray[index].buffer[0];
        fifoArray[index].lostData = 0;
        G8RTOS_InitSemaphore(&fifoArray[index].currentSize, 0); // init to 0 because empty FIFOs should block threads until it has data
        G8RTOS_InitSemaphore(&fifoArray[index].mutex, 1);       // init to 1 because no thread currently is reading from this FIFO

        return 1;
    }
    else
    {
        return 0;
    }
}

int32_t G8RTOS_ReadFIFO(uint32_t index)
{
    G8RTOS_AcquireSemaphore(&fifoArray[index].currentSize);     // ensures there is data to be read (buffer isn't empty)
    G8RTOS_AcquireSemaphore(&fifoArray[index].mutex);           // ensures that FIFO was not currently being read by another thread

    int32_t data = *fifoArray[index].head;                          // data is at head ptr of FIFO
    fifoArray[index].head++;                                        // set head to next in FIFO
    if (fifoArray[index].head == (int32_t*)&fifoArray[index].head)  // if head has gone out of bounds...
    {
        fifoArray[index].head = (int32_t*)&fifoArray[index].buffer; // reset head to start of buffer
    }

    G8RTOS_ReleaseSemaphore(&fifoArray[index].mutex);           // signal other threads that the FIFO is done being read

    return data;
}

int G8RTOS_WriteFIFO(uint32_t index, uint32_t data)
{
    //G8RTOS_AcquireSemaphore(&fifoArray[index].mutex);

    if (fifoArray[index].currentSize > MAX_FIFO_SIZE - 1)       // check if current size is at full capacity
    {
        fifoArray[index].currentSize = MAX_FIFO_SIZE - 1;       // discard new data, increment lostData, return error
        fifoArray[index].lostData++;
        return 1;
    }
    else
    {
        *fifoArray[index].tail = data;
        G8RTOS_ReleaseSemaphore(&fifoArray[index].currentSize);     // release currentSize semaphore
        fifoArray[index].tail++;                                        // increment tail
        if(fifoArray[index].tail == (int32_t*)&fifoArray[index].head)   // if tail has gone out of bounds...
        {
            fifoArray[index].tail = fifoArray[index].buffer;            // reset tail to start of buffer
        }


    }

    //G8RTOS_ReleaseSemaphore(&fifoArray[index].mutex);

    return 0;
}


/*********************************************** Public Functions *********************************************************************/

