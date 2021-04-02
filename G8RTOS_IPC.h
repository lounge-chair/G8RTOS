/*
 * G8RTOS_IPC.h
 */

#ifndef G8RTOS_G8RTOS_IPC_H_
#define G8RTOS_G8RTOS_IPC_H_

/***************************************************** Includes ***********************************************************************/

#include <G8RTOS.h>

/***************************************************** Includes ***********************************************************************/

/*************************************************** Defines Used *********************************************************************/

#define MAX_FIFOS 4
#define MAX_FIFO_SIZE 16

/*************************************************** Defines Used *********************************************************************/

/************************************************* Structures Used ********************************************************************/

typedef struct FIFO_t
{
    int32_t buffer[MAX_FIFO_SIZE];
    int32_t * head;
    int32_t * tail;
    uint32_t lostData;
    semaphore_t currentSize;
    semaphore_t mutex;
} FIFO_t;

/*********************************************** Structures Used **********************************************************************/

/*********************************************** Public Functions *********************************************************************/

/*
 * Initialize the FIFO structure
 */
int G8RTOS_InitFIFO(uint32_t index);

/*
 * Reads a value from the specified FIFO
 *      - index is the intended FIFO to read from
 *      - returns the data read from the the head
 *
 * NOTE: any thread reading a FIFO must ensure that the FIFOis being written by another
 *       active thread or else the thread will not run and cannot be killed
 */
int32_t G8RTOS_ReadFIFO(uint32_t index);

/*
 * Writes a value to the specified FIFO
 *      - index is the intended FIFO to write to
 *      - data is the value to write to the tail
 *      - returns
 *      - if FIFO is full (buffer > 16) then discard the new data and increment dataLost
 *      - returns error if full buffer
 */
int G8RTOS_WriteFIFO(uint32_t index, uint32_t data);

/*********************************************** Public Functions *********************************************************************/

#endif /* G8RTOS_G8RTOS_IPC_H_ */
