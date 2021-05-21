### The Top Directory: ###

- /src : It contains main.c along with a makefile.
- /obj : Empty initially, but after running it would contain main.o
- /bin : Initially empty, but before execution would contain the executables replace.
- /output: Initially empty, but would contain plot.png and data.dat.
- /src_b : Source code and Dockerfile for part-B (which is incomplete).
- /input : Some test cases.

### Operation Specification: ###

For part-A:

- make compile: This would compile the main.c in /src, and then populate the several other directories.
- make execute i=../input/test1.txt first=FF second=LR third=LF fourth=CL fifth=SC
  to execute all five strategies. Can also use optional arguments or no arguments (to execute all 5 test cases).
- make clean: This would remove the contents of /bin, /obj, and /output. 
