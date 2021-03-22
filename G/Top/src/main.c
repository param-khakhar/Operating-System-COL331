#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <math.h>
#include <time.h>
#include <pthread.h>

#define LIMIT 50
#define CHUNK 100

long long global;
long long limit;
int counts[LIMIT];
double times[LIMIT];

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

bool isPrime(long long int local){
	if(local == 1)
		return false;
	long long int lower = floor(sqrt(local));
	int i = 2;
	while(i <= lower){
		if(local % i == 0){
			return false;
		}
		i++;
	}
	return true;
}

struct arguments{
	long long int low, high, id;
};

void* runSlow(void* args){

	struct timespec start, end;
	clock_gettime(CLOCK_MONOTONIC, &start);

	struct arguments *arg = (struct arguments*) args;
	long long int lower = arg->low;
	long long int higher = arg->high;
	long long int temp = (int)arg->id;

	long long int local = lower;
	while(local <= higher){
		if(isPrime(local)){
			counts[temp]++;
		}
		local++;
	}
	clock_gettime(CLOCK_MONOTONIC, &end);
	double time_taken = ((double)end.tv_sec + 1.0e-9*end.tv_nsec) - ((double)start.tv_sec + 1.0e-9*start.tv_nsec);
	times[temp] = time_taken;
}

void* runFast(int* id){

	long long local;
	long long top;
	struct timespec start, end;
	clock_gettime(CLOCK_MONOTONIC, &start);
	//printf("%d %d\n",*id,global);
	while(global < limit){
		pthread_mutex_lock(&mutex1);
		local = global;
		global = global + CHUNK;
		pthread_mutex_unlock(&mutex1);
		top = local+CHUNK;
		//printf("%d %d %d\n",*id,local,top);
		while(local < top && local < limit){
			if(isPrime(local)){
				counts[*id]++;		
			}
			local++;
		}
	}
	clock_gettime(CLOCK_MONOTONIC, &end);
	double time_taken = ((double)end.tv_sec + 1.0e-9*end.tv_nsec) - ((double)start.tv_sec + 1.0e-9*start.tv_nsec);
	times[*id] = time_taken;
}

int main(int argc, char* argv[]) 
{

	FILE* dataSlow;
	FILE* dataFast;

	dataSlow = fopen("dataSlow.dat","w");
	dataFast = fopen("dataFast.dat","w");

	global = 1;
	int num_threads = atoll(argv[1]);
	limit = atoll(argv[2]);

	pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t)*num_threads);
	
	long long chunk = limit/num_threads;
	printf("%d %lld %d\n",num_threads,limit,chunk);

	for(int i = 0;i<num_threads; i++){
		threads[i] = (pthread_t)malloc(sizeof(pthread_t));
		struct arguments* temp = (struct arguments*)malloc(sizeof(struct arguments));
		temp->low = i*chunk + 1;
		temp->high = (i+1)*chunk;
		int* j = malloc(sizeof(int));
		*j = i;
		temp->id = *j;
		pthread_create(&threads[i], NULL, runSlow, (void*)temp);
	}
	for(int i=0;i<num_threads;i++){
		pthread_join(threads[i], NULL);
	}
	for(int i=0;i<num_threads;i++){
		fprintf(dataSlow,"%.1f %f %d\n",i*1.5,times[i],counts[i]);
		counts[i] = 0;
		times[i] = 0.0;
	}
	printf("\n");
	global = 1;
	printf("%d\n",limit);
	for(int i = 0;i<num_threads;i++){
		int* j = malloc(sizeof(int));
		*j = i;
		pthread_create(&threads[i], NULL, runFast, j);
	}

	for(int i=0;i<num_threads;i++){
		pthread_join(threads[i], NULL);
	}

	for(int i=0;i<num_threads;i++)
		fprintf(dataFast,"%.1f %f %d\n",i*1.5 + 0.5,times[i],counts[i]);

	char* commands [] = {"set title \"Comparison For 10 Threads and Limit 1e08\"", "set xtics (\"T0\" 0.25, \"T1\" 1.75, \"T2\" 3.25, \"T3\" 4.75, \"T4\" 6.25, \"T5\" 7.75, \"T6\" 9.25, \"T7\" 10.75, \"T8\" 12.25, \"T9\" 13.75)","set boxwidth 0.5", "set style fill solid", "plot \"dataSlow.dat\" using 1:2 title \"Static Interval Assignment\" with boxes ls 1 lt rgb \"#406090\",\"dataFast.dat\" using 1:2 title \"Dynamically Check Next\" with boxes ls 2 lt rgb \"#40FF00\"","set xlabel \"Thread ID\"", "set ylabel \"Time (seconds)\"","exit"};

	FILE* gnuplotPipe = popen ("gnuplot -persistent", "w");
	char setOut [100] = "set output \"";
	char* result = strcat(setOut,argv[3]);
	char* res1 = strcat(result,"\"");
	char res [10000] = "";
	strcpy(res,res1);
	printf("%s\n",res);
	fprintf(gnuplotPipe, "%s\n","set yrange [0:150]");
	fprintf(gnuplotPipe, "%s\n","set term png");
	fprintf(gnuplotPipe, "%s\n",res);
	
   	for (int i=0; i < strlen(commands); i++){
    	fprintf(gnuplotPipe, "%s \n", commands[i]); 
    }	
	fclose(dataFast);
	fclose(dataSlow);
	return 0; 
} 