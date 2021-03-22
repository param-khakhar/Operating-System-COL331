## Prime Number Generation Using POSIX Threads ##

### The Top Directory: ###

- /src : It contains main.c, which is the source code for the assignment, along with a makefile.
- /obj : It contains main.o, the object file for the assignment along with a makefile.
- /bin : It contains primeChecker, the binary executable for the assignment along with a makefile.
- /lib : Currently empty.
- /include: Currently empty.
- /output: It contains dataFast.dat, dataSlow.dat, and plot.png, which are the generated on executing the executable, primeChecker.
- /extras: This directory contains .dat files along with .png files which are the results of earlier experiments, which involved generating prime numbers till 1e07 and 1e08. This folder also contains the plot on varying the number of threads.

### Operation Specification: ###

- Typing "make" in the Top Directory would result in the execution of the binary file for 10 threads and limit 1e06.
- In order to compile the source code, use "make clean".
- "make clean" would delete src/main.c, obj/main.o, bin/primeChecker, output/*, extras/*.
- Typing "make plot" would generate the plot for comparison between different number of threads.

### Results: ###

#### Static Allocation ####

|Thread ID   |Time in Seconds |Primes Computed|           
|------------|----------------|---------------|
|    0       |    0.046759    |     9592      |
|    1       |    0.067099    |     8392      |
|    2       |    0.083444    |     8013      |
|    3       |    0.092718    |     7863      |
|    4       |    0.102055    |     7678      |
|    5       |    0.107718    |     7560      |
|    6       |    0.112243    |     7445      |
|    7       |    0.114406    |     7408      |
|    8       |    0.117090    |     7323      |
|    9       |    0.103994    |     7224      |

#### Dynamic Allocation ####

|Thread ID   |Time in Seconds |Primes Computed|           
|------------|----------------|---------------|
|    0       |    0.114702    |     9793      |
|    1       |    0.114705    |     9905      |
|    2       |    0.114469    |     9896      |
|    3       |    0.113936    |     9695      |
|    4       |    0.058754    |     5475      |
|    5       |    0.114649    |     9841      |
|    6       |    0.113432    |     9796      |
|    7       |    0.056074    |     4357      |
|    8       |    0.061774    |     5676      |
|    9       |    0.052889    |     4064      |


The time taken by the load balanced approach, dynamic scheduling is roughly equal for all the threads whereas the 
the time taken for the static interval assignment approach is lesser for initial threads and more for later ones.
This can be attributed to the amount of computation to be carried out for both the approaches. The uneven times for
the load balanced approach could be explained by the uneven distribution of computation. On experimentation, it was
observed that, better results are observed if we decrease the number of threads, or increase the limit.

On varying the number of threads, it was observed that increasing in the number of threads resulted in less time taken
for the part involving computation of primes and consequently the execution time of the program would be less as well.

