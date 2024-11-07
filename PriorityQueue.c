/**
 * @file PriorityQueue.c
 * @brief A simple Priority Queue implemented for HttpResponse.
 *
 * A min-heap implemented specifically for HttpResponse structs
 * as defined in HttpResponse.h. 
 * The functions are not thread-safe, and mutexes must be used before
 * accessing the PQ.
 *
 * Modified from 
 * https://www.geeksforgeeks.org/c-program-to-implement-priority-queue/
 * 
 * @author syam1270
 * @author Joshua Hellauer
 *
 * @date 2024-11-04
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "PriorityQueue.h"
#include "HttpResponse.h"
#include <string.h>
#include <limits.h>

/**
 * @brief Simple swap function
 *
 * @param a The first HttpResponse
 * @param b The second HttpResponse
 */
void swap(HttpResponse* a, HttpResponse* b)
{
    HttpResponse temp = *a;
    *a = *b;
    *b = temp;
}

/**
 * @brief Compare two timepsecs.
 *
 * Used to maintain min-heap property.
 * Returns 1 if t1 > t2
 *         -1 if t1 < t2
 *          0 if t1 == t2.
 *
 * @param time1 A pointer to the first timespec.
 * @param time2 A pointer to the second timespec.
 * @return 1, 0, or -1.
 */
int compare_timespec(struct timespec *time1, struct timespec *time2) {
    if (time1->tv_sec > time2->tv_sec) {
        return 1;
    } else if (time1->tv_sec < time2->tv_sec)    
    {
        return -1;
    } else 
    {
        return time1->tv_nsec > time2->tv_nsec ? 1 : (time1->tv_nsec < time2->tv_nsec ? -1 : 0);
    }
}

/**
 * @brief Search a PriorityQueue.
 *
 * @param pq The Priority Queue to search.
 * @param target The filename to match.
 * @return An HttpResponse with filename target, or NULL.
 */
HttpResponse* search(PriorityQueue* pq, char* target) 
{
    HttpResponse* ret = NULL;

    for(int i = 0; i < pq->size; i++) {
        if(strcmp(pq->items[i]->filename, target) == 0) {
            ret = pq->items[i];
            clock_gettime(CLOCK_REALTIME, &(pq->items[i]->access_time));
            heapifyUp(pq, i);
            break;
        }
    }

    return ret;
}


/**
 * @brief Heapify up.
 * 
 * Keep pushing PQ[index] up further until the heap
 * property is restored.
 * 
 * @param pq The Priority Queue.
 * @param index The target index being operated on.
 */
void heapifyUp(PriorityQueue* pq, int index)
{
    if (index
        && compare_timespec(&(pq->items[(index - 1) / 2]->access_time), &(pq->items[index]->access_time)) == -1) {
        swap(pq->items[(index - 1) / 2],
             pq->items[index]);
        heapifyUp(pq, (index - 1) / 2);
    }
}

/**
 * @brief Enqueue a new HttpResponse into the PQ.
 *
 * If the PQ is at max capacity, enqueue() will also remove
 * the entry with the oldest request time. This could have been 
 * better compartmentalized into a dequeue() function.  
 *
 * @param pq The Priority Queue which shall be inserted into.
 * @param value The HttpResponse struct to insert into the PQ.
 */
void enqueue(PriorityQueue* pq, HttpResponse* value)
{
    if (pq->size == MAX) {
        // travers the first size/2 nodes and remove the smallest
        int amt = pq->size / 2 + 1;
        int min_index = MAX - 1;
        struct timespec min;
        min.tv_sec = LONG_MAX;
        min.tv_nsec = LONG_MAX;
        for(int i = 0; i < amt; i++) {
            if(compare_timespec(&(pq->items[pq->size - i - 1]->access_time), &min) == -1) {
                min = pq->items[pq->size - i - 1]->access_time;
                min_index = pq->size - i - 1;
            }
        }
        printf("Freeing %s\n", pq->items[min_index]->filename);
        free(pq->items[min_index]->filename);
        free(pq->items[min_index]->response);
        free(pq->items[min_index]);
        pq->items[min_index] = value;
        heapifyUp(pq, min_index);
    } else {
        pq->items[pq->size++] = value;
        heapifyUp(pq, pq->size - 1);
    }
}