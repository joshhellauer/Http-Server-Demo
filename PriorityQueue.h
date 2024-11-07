#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include "HttpResponse.h"

// Define maximum size of the priority queue
#define MAX_QUEUE_SIZE 5

// Define PriorityQueue structure
typedef struct PriorityQueue {
    HttpResponse* items[MAX];
    int size;
} PriorityQueue;

HttpResponse* search(PriorityQueue *pq, char *filename);

// Define swap function to swap two integers
void swap(HttpResponse* a, HttpResponse* b);

// Define heapifyUp function to maintain heap property
// during insertion
void heapifyUp(PriorityQueue* pq, int index);

// Define enqueue function to add an item to the queue
void enqueue(PriorityQueue* pq, HttpResponse* value);

// Define heapifyDown function to maintain heap property
// during deletion
int heapifyDown(PriorityQueue* pq, int index);

// Define dequeue function to remove an item from the queue
HttpResponse* dequeue(PriorityQueue* pq);

// Define peek function to get the top item from the queue
HttpResponse* peek(PriorityQueue* pq);

#endif