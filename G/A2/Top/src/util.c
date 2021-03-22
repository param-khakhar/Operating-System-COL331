#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>	

#define LIMIT 50
#define CHUNK 100

int main(int argc, char* argv[]) 
{

	char* commands [] = {"set title \"Comparison For 10 Threads and Limit 1e06\"", "set xtics (\"2\" 0.25, \"4\" 1.75, \"6\" 3.25, \"8\" 4.75, \"10\" 6.25)","set boxwidth 0.5", "set style fill solid", "plot '../extras/varThreads.dat' every 2 using 1:2 title \"Dynamically Check Next (Chunk Size = 100)\" with boxes ls 1 lt rgb \"#40FF00\",\'../extras/varThreads.dat' every 2::1 using 1:2 title \"Static Interval Assignment\" with boxes ls 2 lt rgb \"#406090\"","exit"};

	FILE* gnuplotPipe = popen ("gnuplot -persistent", "w");
	char setOut [100] = "set output \"../extras/";
	char* result = strcat(setOut,argv[1]);
	char* res1 = strcat(result,"\"");
	char res [10000] = "";
	strcpy(res,res1);
	//printf("%s\n",res);
	fprintf(gnuplotPipe, "%s\n","set xlabel \"# Threads\"");
	fprintf(gnuplotPipe, "%s\n","set ylabel \"Time for Prime-Computation only in Seconds\"");
	//fprintf(gnuplotPipe, "%s\n","set yrange [0:110]");
	fprintf(gnuplotPipe, "%s\n","set term png");
	fprintf(gnuplotPipe, "%s\n",res);
	
   	for (int i=0; i < strlen(commands); i++){
    	fprintf(gnuplotPipe, "%s \n", commands[i]); 
    }
	return 0; 
} 