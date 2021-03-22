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
	primes = fopen("../output/output.txt","w");
	dataSlow = fopen("../output/dataSlow.dat","w");
	dataFast = fopen("../output/dataFast.dat","w");

	global = 1;
	int num_threads = atoll(argv[2]);
	limit = atoll(argv[1]);
	
	pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t)*num_threads);

	primeF = (int*)calloc(limit, sizeof(int));
	primeS = (int*)calloc(limit, sizeof(int));
	
	long long chunk = limit/num_threads;
	//printf("%d %lld %d\n",num_threads,limit,chunk);

	clock_gettime(CLOCK_MONOTONIC, &start);
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
	clock_gettime(CLOCK_MONOTONIC, &end);
	clock_slow = ((double)end.tv_sec + 1.0e-9*end.tv_nsec) - ((double)start.tv_sec + 1.0e-9*start.tv_nsec);
	
	for(int i=0;i<num_threads;i++){
		fprintf(dataSlow,"%.1f %f %d\n",i*1.0 + 1,times[i],counts[i]);
		//clock_slow += times[i];
		counts[i] = 0;
		times[i] = 0.0;
	}
	global = 1;

	clock_gettime(CLOCK_MONOTONIC, &start);
	for(int i = 0;i<num_threads;i++){
		int* j = malloc(sizeof(int));
		*j = i;
		pthread_create(&threads[i], NULL, runFast, j);
	}

	for(int i=0;i<num_threads;i++){
		pthread_join(threads[i], NULL);
	}

	clock_gettime(CLOCK_MONOTONIC, &end);
	clock_fast = ((double)end.tv_sec + 1.0e-9*end.tv_nsec) - ((double)start.tv_sec + 1.0e-9*start.tv_nsec);

	for(int i=0;i<num_threads;i++){
		fprintf(dataFast,"%.2f %f %d\n",i*1.0 + 1.25,times[i],counts[i]);
	}
	fprintf(primes,"%s\n","Naive Approach\n");
	for(int i=0;i<limit;i++){
		if(primeS[i])
			fprintf(primes,"%d\n",i);
	}
	fprintf(primes,"%s\n","Load Balanced Approach\n");
	for(int i=0;i<limit;i++){
		if(primeF[i])
			fprintf(primes,"%d\n",i);
	}


	float xticend = num_threads;

	int rangeend = (int)xticend + 1;

	char* commands [] = {"set title \"Comparison for %d Threads and Limit %d\"", "set xtics 0, 1, %f", "set xrange [0:%d]", "set boxwidth 0.25", "set style fill solid", "plot \"../output/dataSlow.dat\" using 1:2 title \"Static Interval Assignment\" with boxes ls 1 lt rgb \"#406090\",\"../output/dataFast.dat\" using 1:2 title \"Dynamically Check Next (Chunk Size = 100)\" with boxes ls 2 lt rgb \"#40FF00\"","exit"};

	FILE* gnuplotPipe = popen ("gnuplot -persistent", "w");

	char setOut [100] = "set output \"../output/";
	char* result = strcat(setOut,argv[3]);
	char* res1 = strcat(result,"\"");
	char res [10000] = "";
	strcpy(res,res1);

	fprintf(gnuplotPipe, "%s\n","set xlabel \"Thread ID\"");
	fprintf(gnuplotPipe, "%s\n","set ylabel \"Time in Seconds\"");
	fprintf(gnuplotPipe, "set yrange [0:%f]\n", maxTime + maxTime/5);
	fprintf(gnuplotPipe, "%s\n","set term png");
	fprintf(gnuplotPipe, "%s\n",res);
	
   	for (int i=0; i < strlen(commands); i++){
		if(i == 0)
			fprintf(gnuplotPipe, commands[i], num_threads, limit);	
		else if(i == 1)
			fprintf(gnuplotPipe, commands[i], xticend);
		else if(i == 2) 			 	
			fprintf(gnuplotPipe, commands[i], rangeend);
		else		
    		fprintf(gnuplotPipe, commands[i]); 
		fprintf(gnuplotPipe,"\n");
    }	
	fclose(dataFast);
	fclose(dataSlow);
	fclose(primes);
	//printf("Clock-Fast: %f\n",clock_fast/(double)num_threads);
	//printf("Clock-Slow: %f\n",clock_slow/(double)num_threads);

	return 0; 
} 
