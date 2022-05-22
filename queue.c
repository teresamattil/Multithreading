


#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "queue.h"



//To create a queue
queue* queue_init(int size){
	queue * q = (queue *)malloc(sizeof(queue));
	
	if(q==NULL){
		return NULL;
	}
	
	q->maxSize = size; // Declare maximum size of the queue
	q->_size = 0; // declare "real" size = number of elements inside the queue
	
	// Reserve memory for each node being struct of elements
	q->vectOfNodes = (struct element*)malloc(size* sizeof( struct element));
	
	if(q->vectOfNodes ==NULL){
		free(q);
		return NULL;
	}
	//To Enqueue and dequeue:
	q->in = 0;
	q->out= 0;
	
	return q;
}


// To Enqueue an element
int queue_put(queue *q, struct element* x) {
	if (!queue_full(q)){
		q->vectOfNodes[q->in] = *x; // Insert element x at input position 
		q->in = (q->in +1)%q->maxSize;  // Increase input position -> to be circular (in +1)%maxSize so we go back to first position if full
		q->_size++;
		return 0;
	}else{
		perror("Buffer is full");
		return -1;
	}
	
}


// To Dequeue an element.
struct element* queue_get(queue *q) {
	struct element* element=NULL;
	if(!queue_empty(q)){
		
		element = &(q->vectOfNodes[q->out]); // we get addr of element that is going to be dequeued
		q->out = (q->out +1)%q->maxSize; // Increase output position by one
		q->_size--;
		return element;
	}else{
		return NULL;
	}
}


//To check queue state
int queue_empty(queue *q){
	if (q->_size ==0){
		return 1;
	}
	else{
		return 0;
	}
}

int queue_full(queue *q){
	if (q->_size == q->maxSize){
		return 1;
	}
	else{
		return 0;
	}
}

//To destroy the queue and free the resources
int queue_destroy(queue *q){
	free(q->vectOfNodes);
	free(q);
	return 0;
}
