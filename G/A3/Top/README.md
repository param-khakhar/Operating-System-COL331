## myThread Library ##

### The Top Directory: ###

- /src : It contains matrixMult.c, boundedBuffer.c, myThread.c and the test files, along with a makefile.
- /obj : Empty initially, but after running it would contain myThread.o
- /bin : Initially empty, but before execution would contain the executables matrixMult, boundedBuffer, and the test files.
- /lib : Currently empty, but would contain libmyThread.so
- /include: Header file, myThread.h
- /output: Initially empty, but would contain plot.png.
- /data : Contains myThread.dat and pThread.dat for plotting graphs.
- /extras: This directory contains .dat files along with .png files which are the results of earlier experiments, which involved generating prime numbers till 1e07 and 1e08. This folder also contains the plot on varying the number of threads.

### Operation Specification: ###

- make matrix_mult: This would compile matrixMult.c and execute the executable with 15 threads.
- make run_matrix_mult: This would only execute the executable matrixMult with 15 threads.
- make boundedBuffer: This would compile boundedBuffer.c and execute the executable with 2 consumers, producers and 3 inventory size.
- make run_boundedBuffer: This would execute the executable with 2 consumers, producers and 3 inventory size.
- make all: Compile all the source files, send the newly generated files to their respective folders.
- make run_test(1 - 7): It would compile the individual tests, and then execute.
- make clean: This would remove the contents of /bin, /lib, /obj, and /output. 