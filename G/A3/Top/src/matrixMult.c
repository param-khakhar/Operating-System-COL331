#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include "../include/myThread.h"

#define LIMIT 40

double times[LIMIT];

int row;
int total;

double clock_slow;
double clock_fast;
double clock_seq;
// struct timespec start, end;

clock_t start, end;
double cpu_time_used;

myMutexLock lock;
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

// int** A;
// int ** B;
// int ** result;
int a, b, c;

int A[1005][1005];
int B[1005][1005];
int result[1005][1005];

void* multiplyM(void* d){
	int curr;
	while(row < total){
		acquire(&lock);
			curr = row;
			row += 1;
		release(&lock);
		for(int i=0;i<1000;i++){
			int temp = 0;
			for(int j=0;j<1000;j++){
				temp = temp + (A[curr][j]) * (B[j][i]);
 			}
			result[curr][i] = temp;
		}
	}
	return NULL;
}

void* multiplyP(void* d){
	int curr;
	while(row < total){
		pthread_mutex_lock(&mutex1);
			curr = row;
			row += 1;
		pthread_mutex_unlock(&mutex1);
		for(int i=0;i<1000;i++){
			int temp = 0;
			for(int j=0;j<1000;j++){
				temp = temp + (A[curr][j]) * (B[j][i]);
 			}
			result[curr][i] = temp;
		}
	}
	return NULL;
}

void multiplySeq(){

	for(int k=0;k<a;k++){
		for(int i=0;i<1000;i++){
			int temp = 0;
			for(int j=0;j<1000;j++){
				temp = temp + (A[k][j]) * (B[j][i]);
 			}
			result[k][i] = temp;
		}
	}
	return;
}

int main(int x, char* argv[]){

	row = 0;

	clock_fast = 0.0;
	clock_slow = 0.0;
	clock_seq = 0.0;

	a = 1000;
	b = 1000;
	c = 1000;

	total = a;

	// A = (int**)malloc(sizeof(int*)*b*a);
	// B = (int**)malloc(sizeof(int*)*c*b);
	// result = (int**)malloc(sizeof(int*)*c*a);

	// for(int i=0;i<a;i++){
	// 	A[i] = malloc(sizeof(int)*b);
	// 	result[i] = malloc(sizeof(int)*c);
	// }

	// for(int i=0;i<b;i++){
	// 	B[i] = malloc(sizeof(int)*c);
	// }

	for(int i=0;i<a;i++){
		for(int j=0;j<c;j++){
			result[i][j] = 0;
		}
	}

	for(int i=0;i<a;i++){
		for(int j=0;j<b;j++){
			A[i][j] = 1;
		}
	}

	for(int i=0;i<b;i++){
		for(int j=0;j<c;j++){
			B[i][j] = 2;
		}
	}

	int num_threads = atoi(argv[1]);

	// clock_gettime(CLOCK_REALTIME, &start);	
	start = clock();

	multiplySeq();

	// clock_gettime(CLOCK_REALTIME, &end);
	end = clock();

	clock_seq = ((double) (end - start)) / CLOCKS_PER_SEC;

	// clock_seq = ((double)end.tv_sec + 1.0e-9*end.tv_nsec) - ((double)start.tv_sec + 1.0e-9*start.tv_nsec);

	myThread** threads = (myThread**)malloc(sizeof(myThread*)*num_threads);
	pthread_t* pthreads = (pthread_t*)malloc(sizeof(pthread_t)*num_threads);

	int sent = 200;
	int* d = &sent;

	row = 0;

	// clock_gettime(CLOCK_MONOTONIC, &start);
	start = clock();

	for(int i = 0;i<num_threads; i++){
		pthreads[i] = (pthread_t)malloc(sizeof(pthread_t));
		pthread_create(&pthreads[i], NULL, multiplyP, (void*)d);
	}

	for(int i=0;i<num_threads;i++){
		pthread_join(pthreads[i], NULL);
	}
	
	end = clock();

	// printf("Here\n");
	// // clock_gettime(CLOCK_MONOTONIC, &end);
	clock_fast = ((double) (end - start)) / CLOCKS_PER_SEC;

	row = 0;

	// // clock_gettime(CLOCK_MONOTONIC, &start);

	start = clock();

	for(int i=0;i<num_threads;i++){
		threads[i] = (myThread*)malloc(sizeof(myThread));
		myThread_create(threads[i], NULL, multiplyM, (void*)d);
	}

	for(int i=0;i<num_threads;i++){
		myThread_join(threads[i], NULL);
	}
	end = clock();
	// // clock_gettime(CLOCK_MONOTONIC, &end);
	clock_slow = ((double) (end - start)) / CLOCKS_PER_SEC;
	// printf("Here\n");

	printf("Num-Threads: %d\n", num_threads);
	printf("Sequential Time: %f\n", clock_seq);
	printf("Pthread - Time: %f\n", clock_fast);
	printf("myThread - Time: %f\n", clock_slow);
	printf("pThread-Overhead-Time: %f\n",clock_fast-clock_seq);
	printf("myThread-Overhead - Time: %f\n",clock_slow - clock_seq);

	float xticend = 40;

	int rangeend = (int)xticend + 1;

	char* commands [] = {"set title \"Comparison between myThread and pThread for Square Matrix %d\"", "set xtics 0, 5, %f", "set xrange [0:%d]", "set boxwidth 1", "set style fill solid", "plot \"../data/myThread.dat\" using 1:2 title \"myThread\" with boxes ls 1 lt rgb \"#406090\",\"../data/pThread.dat\" using 1:2 title \"pThread\" with boxes ls 2 lt rgb \"#40FF00\"","exit"};

	FILE* gnuplotPipe = popen ("gnuplot -persistent", "w");

	char setOut [100] = "set output \"../output/";
	char* result = strcat(setOut,argv[2]);
	char* res1 = strcat(result,"\"");
	char res [10000] = "";
	strcpy(res,res1);

	fprintf(gnuplotPipe, "%s\n","set xlabel \"Number of Threads\"");
	fprintf(gnuplotPipe, "%s\n","set ylabel \"Time in Seconds\"");
	fprintf(gnuplotPipe, "set yrange [0:%f]\n", 10.0);
	fprintf(gnuplotPipe, "%s\n","set term png");
	fprintf(gnuplotPipe, "%s\n",res);
	
   	for (int i=0; i < sizeof(commands)/sizeof(commands[0]); i++){
		if(i == 0)
			fprintf(gnuplotPipe, commands[i], 1000);	
		else if(i == 1)
			fprintf(gnuplotPipe, commands[i], xticend);
		else if(i == 2) 			 	
			fprintf(gnuplotPipe, commands[i], 42);
		else		
    		fprintf(gnuplotPipe, commands[i]); 
		fprintf(gnuplotPipe,"\n");
    }
	return 0; 
} 