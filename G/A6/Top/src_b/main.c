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

int* primeF;
int* primeS;
FILE* primes;

double maxTime = 0.0;

long long global;
long long limit;
int counts[LIMIT];
double times[LIMIT];

double clock_slow = 0.0;
double clock_fast = 0.0;

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

	//clock_gettime(CLOCK_MONOTONIC, &start);
	clockid_t threadClockId;
	pthread_getcpuclockid(pthread_self(), &threadClockId);
	clock_gettime(threadClockId, &start);

	struct arguments *arg = (struct arguments*) args;
	long long int lower = arg->low;
	long long int higher = arg->high;
	long long int temp = (int)arg->id;

	long long int local = lower;
	while(local <= higher){
		if(isPrime(local)){
			primeS[local] = 1;
			counts[temp]++;
		}
		local++;
	}

	clock_gettime(threadClockId, &end);
	double time_taken = ((double)end.tv_sec + 1.0e-9*end.tv_nsec) - ((double)start.tv_sec + 1.0e-9*start.tv_nsec);
	times[temp] = time_taken;
	if(time_taken > maxTime)
		maxTime = time_taken;
}

void* runFast(int* id){

	long long local;
	long long top;
	struct timespec start, end;
	clockid_t threadClockId;
	pthread_getcpuclockid(pthread_self(), &threadClockId);
	clock_gettime(threadClockId, &start);
	//clock_gettime(CLOCK_MONOTONIC, &start);
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
				//fprintf(primes,"%d\n",local);
				primeF[local] = 1;
				counts[*id]++;		
			}
			local++;
		}
	}
	//clock_gettime(CLOCK_MONOTONIC, &end);
	clock_gettime(threadClockId, &end);
	double time_taken = ((double)end.tv_sec + 1.0e-9*end.tv_nsec) - ((double)start.tv_sec + 1.0e-9*start.tv_nsec);
	times[*id] = time_taken;
	if(time_taken > maxTime)
		maxTime = time_taken;
}

int main(int argc, char* argv[]) 
{

	struct timespec start, end;

	FILE* dataSlow;
	FILE* dataFast;

	global = 1;
	// int num_threads = atoll(argv[2]);
	// limit = atoll(argv[1]);

	int num_threads = 5;
	limit = 100000000;
	int i;
	pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t)*num_threads);

	primeF = (int*)calloc(limit, sizeof(int));
	primeS = (int*)calloc(limit, sizeof(int));
	
	long long chunk = limit/num_threads;
	//printf("%d %lld %d\n",num_threads,limit,chunk);

	clock_gettime(CLOCK_MONOTONIC, &start);
	for(i = 0;i<num_threads; i++){
		threads[i] = (pthread_t)malloc(sizeof(pthread_t));
		struct arguments* temp = (struct arguments*)malloc(sizeof(struct arguments));
		temp->low = i*chunk + 1;
		temp->high = (i+1)*chunk;
		int* j = malloc(sizeof(int));
		*j = i;
		temp->id = *j;
		pthread_create(&threads[i], NULL, runSlow, (void*)temp);
	}
	for(i=0;i<num_threads;i++){
		pthread_join(threads[i], NULL);
	}
	clock_gettime(CLOCK_MONOTONIC, &end);
	clock_slow = ((double)end.tv_sec + 1.0e-9*end.tv_nsec) - ((double)start.tv_sec + 1.0e-9*start.tv_nsec);
	// for(int i=0;i<num_threads;i++){
	// 	fprintf(dataSlow,"%.1f %f %d\n",i*1.0 + 1,times[i],counts[i]);
	// 	//clock_slow += times[i];
	// 	counts[i] = 0;
	// 	times[i] = 0.0;
	// }
	// global = 1;

	clock_gettime(CLOCK_MONOTONIC, &start);
	for(i = 0;i<num_threads;i++){
		int* j = malloc(sizeof(int));
		*j = i;
		pthread_create(&threads[i], NULL, runFast, j);
	}

	for(i=0;i<num_threads;i++){
		pthread_join(threads[i], NULL);
	}

	clock_gettime(CLOCK_MONOTONIC, &end);
	clock_fast = ((double)end.tv_sec + 1.0e-9*end.tv_nsec) - ((double)start.tv_sec + 1.0e-9*start.tv_nsec);

	printf("Limit: %d\n",limit);
	printf("Num-threads: %d\n", num_threads);
	printf("Clock-Fast: %f\n",clock_fast/(double)num_threads);
	printf("Clock-Slow: %f\n",clock_slow/(double)num_threads);

	return 0; 
} 
