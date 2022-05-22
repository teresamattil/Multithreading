#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/stat.h>
#include <pthread.h>
#include "queue.h"
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <string.h>
#include <math.h>


// Global variables needed on producers and consumers, so we don't have to pass them as arguments
struct element *data; 
queue *buff;
int num; // Number of operations (first line of the input file)
int result=0; // Var to store the total cost 

typedef struct arguments{
	//fileInfo *arr;
	int first, last;
}arguments;



pthread_cond_t non_full; /* can we add more elements? */
pthread_cond_t non_empty; /* can we remove elements? */
pthread_mutex_t mutex;



void *producer(void *args){  
  
	arguments *prodArgs = (arguments *)args;
  	//fileInfo *data = prodArgs->arr;
    	
  	/* DESCRIPTION:
  		The circular queue buff ahs already been declared as a global variable and initialized in the main function. Now, we will enqueue the objects of element structure that are in the array of struct data.	
  	*/
  	
  	
  	
	for(int i =prodArgs->first; i<prodArgs->last; i++) {			
		if (pthread_mutex_lock(&mutex)!=0){// access to buffer
			perror("Error on execution of pthread_mutex_lock()");
			exit(1);
		}
		while (queue_full(buff)){ // when buffer is full wait to be emptyed 

			if (pthread_cond_wait(&non_full, &mutex)!=0){ 
			perror("Error on execution of pthread_cond_wait()");
			exit(4);
			}	
		}
		
   		if (queue_put(buff,&data[i])<0){ // We store the element data[i] in the buffer

			perror("Error adding data to the shared circular buffer");
		}
			 
		if (pthread_cond_signal(&non_empty)!=0){ // buffer is not empty
			perror("Error on execution of pthread_cond_signal()");
			exit(3);
		}

  		if (pthread_mutex_unlock(&mutex)!=0){
			perror("Error on execution of pthread_mutex_unlock()");
			exit(2);
		}
    	
    	}
	pthread_exit(NULL);
	return NULL;
}

// *********** Consumer *********
void *consumer(void *args){   
  	arguments *conArgs = (arguments *)args;
  	struct element *elem; 
  	int value;

	/* DESCRIPTION:
  		Extract the elements from the circular queue buffer and by knowing its type add the result to the partial cost being calculated
  	*/
  
    	for(int i =conArgs->first; i<conArgs->last; i++) {
		if (pthread_mutex_lock(&mutex)!=0){// access to buffer
			perror("Error on execution of pthread_mutex_lock()");
			exit(1);
		}
	 	while (queue_empty(buff)){ // when buffer is empty wait to be filled 
			
			if (pthread_cond_wait(&non_empty, &mutex)!=0){ 
			perror("Error on execution of pthread_cond_wait()");
			exit(4);
			}	
		}

      		elem = queue_get(buff); // extract the element from the queue

		if (elem == NULL){

		perror("Error extracting the element from the queue");

		}
      		
      		switch(elem->type){ // correspond a value depending on the type of the element
      			case 1:
      				value=3;
      				break;
      			case 2:
      				value=6;
      				break;
      			case 3:
      				value=15;
      				break;
      		}

		// after knowing its type and its assigned value add to the partial cost

      		result = result + (value * elem->time); // updates the partial cost of each thread
      		
		if (pthread_cond_signal(&non_full)!=0){ // buffer is not full
			perror("Error on execution of pthread_cond_signal()");
			exit(3);
		}

      		if (pthread_mutex_unlock(&mutex)!=0){
			perror("Error on execution of pthread_mutex_unlock()");
			exit(2);
		}
    	} // End of for loop
	pthread_exit(NULL);
	return NULL;	
}



int main (int argc, const char * argv[] ) {
	

	// Reading Input arguments
	if (argc !=5){ // check correct number of inputs
		perror("The structure of the command is: /.calculator <file_name> <num_prod> <num_cons> <buff_size>\n");
		return -1;
	}

	const char *fileName = argv[1];
	int prods = atoi(argv[2]);
	int cons = atoi(argv[3]);
	int bsize = atoi(argv[4]);
		
	if(prods<=0){
		perror("Invalid number of producers");
		return -1;
	}
	
	if(cons<=0){
		perror("Invalid number of consumers");
		return -1;
	}
	if(bsize<=0){
		perror("Invalid size of buffer");
		return -1;
	}
	
	// ***************** Loading data from input file into memory ****************
	// Reading the input file
	FILE *fidin = NULL;
	
	fidin = fopen(fileName,"r"); // Open the file
	if (!fidin){
		perror("Error opening the file");
		return 1;
	}
	if(fscanf(fidin,"%d", &num)!=1){// first get number of processes
		perror("Error reading number of processes");
		return -1;
	} 
	
	
	
	data =(struct element*) malloc(num*sizeof(struct element)); // Rerserve memory for array of structure

	if(data==NULL){
		perror("Error allocating data structure");
		free(data);
		return -1;
	}

	int id; // get the id, but we won't use it
	for (int i =0; i<num; i++){ // Store the data of the file int the array
		if(fscanf(fidin, "%d %d %d", &id, &data[i].type, &data[i].time) ==3);
		else{
			perror("Error reading the file");
			return 1;
		}
	}
	fclose(fidin);
	
	
	// ******** Distributing the file load ************** 
	
	
	pthread_t pro[prods],con[cons];
	
	// Check by error handling the initialisation of the mutex and conditional variables

	if (pthread_mutex_init(&mutex, NULL)<0){
		perror("Error initializing the mutex\n");
		free(data);
		return -1;
	}

	if (pthread_cond_init(&non_full, NULL)<0){
		perror("Error initializing the condition variable\n");
		free(data);
		pthread_mutex_destroy(&mutex);
		return -1;
	}

	if (pthread_cond_init(&non_empty, NULL)<0){
		perror("Error initializing the condition variable\n");
		free(data);
		pthread_mutex_destroy(&mutex);
		pthread_cond_destroy(&non_full);
		return -1;
	}

	
	arguments *argsP = malloc(num*sizeof(arguments));
	//Check the allocation in memory is correct

	if(argsP == NULL){
		perror("Error allocating argsP");
		free(data);
		pthread_mutex_destroy(&mutex);
		pthread_cond_destroy(&non_full);
		pthread_cond_destroy(&non_empty);
		return -1;
	}	


	arguments *argsC = malloc(num*sizeof(arguments));
	//Check the allocation in memory is correct

	if(argsC == NULL){
		perror("Error allocating argsP");
		free(data);
		free(argsP);
		pthread_mutex_destroy(&mutex);
		pthread_cond_destroy(&non_full);
		pthread_cond_destroy(&non_empty);
		return -1;
	}

	
	//Initializing the queue buff (already declared on global variables)
	buff = queue_init(num);
	
	if(buff == NULL){
		perror("Error allocating memory for queue");
		pthread_mutex_destroy(&mutex);
		pthread_cond_destroy(&non_full);
		pthread_cond_destroy(&non_empty);
		free(data);
		free(argsP);
		free(argsC);
		return -1;
	}
	
	// for each thread a specific range of elements is given depending on the number of operations and the number of producers. All of them are equally distributed

 	for(int i = 0; i < prods; i++) {
  		
  		if (i!= prods-1){
  			argsP[i].first = ceil( i*num/prods );
  			argsP[i].last  = ceil ( (i+1)*num/prods );
  		} else{
  			argsP[i].first = ceil( i*num/prods );
  			argsP[i].last  = num;
  		}
  		
		// creates a thread for each number of producers with parameter the range of elements it must work on
  		
		if (pthread_create(&pro[i], NULL, (void *)producer, &argsP[i])!=0){ 
			perror("Error on execution of creating a thread");
			pthread_mutex_destroy(&mutex);
			pthread_cond_destroy(&non_full);
			pthread_cond_destroy(&non_empty);
			free(data);
			free(argsP);
			free(argsC);
			queue_destroy(buff);
			exit(5); 
			}
    	}


	// for each thread a specific range of elements is given depending on the number of operations and the number of consumers. All of them are equally distributed
  
  	for(int i = 0; i < cons; i++) { 
	  	if (i!= cons-1){
	  		argsC[i].first = ceil( i*num/cons );
	  		argsC[i].last  = ceil ( (i+1)*num/cons );
	  	} else{
	  		argsC[i].first = ceil( i*num/cons );
	  		argsC[i].last  = num;
	  	}
	  	
		// creates a thread for each number of producers with parameter the range of elements it must work on
		
		if (pthread_create(&con[i], NULL, (void *)consumer, &argsC[i])!=0){ 
			perror("Error on execution of creating a thread");
			pthread_mutex_destroy(&mutex);
			pthread_cond_destroy(&non_full);
			pthread_cond_destroy(&non_empty);
			free(data);
			free(argsP);
			free(argsC);
			queue_destroy(buff);
			exit(5); 
			}	

        	
    	}

  	for(int i = 0; i < prods; i++) {
		
		if (pthread_join(pro[i], NULL)!=0){ 
			perror("Error on execution of joining a thread");
			pthread_mutex_destroy(&mutex);
			pthread_cond_destroy(&non_full);
			pthread_cond_destroy(&non_empty);
			free(data);
			free(argsP);
			free(argsC);
			queue_destroy(buff);
			exit(6); 
		}
    	}
  
  	for(int i = 0; i < cons; i++) {
        	
		if (pthread_join(con[i], NULL)!=0){ 
			perror("Error on execution of joining a thread");
			pthread_mutex_destroy(&mutex);
			pthread_cond_destroy(&non_full);
			pthread_cond_destroy(&non_empty);
			free(data);
			free(argsP);
			free(argsC);
			queue_destroy(buff);
			exit(6); 
		}
    	}

	// destroy all mutex and conditional variabñes
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&non_full);
	pthread_cond_destroy(&non_empty);
	
	// End of producers-consumers part
	
	printf("Total: %d euros.\n",result);
	
	
	// destroy the circular queue buffer
	queue_destroy(buff);

	// free the space of allocated memory
	free(data);
	free(argsP);
	free(argsC);
	
    return 0;
}
