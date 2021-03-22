.PHONY := clean

all:
	@(cd src; make -s bB)
	@(cd src; make -s mM)
	@(cd src; make -s tests)

boundedBuffer:
	@(cd src; make -s bB)
	@(cd bin; make -s bB)

matrix_mult:
	@(cd src; make -s mM)
	@(cd bin; make -s mM);

execute:
	@(cd bin; $(MAKE))

run_matrix_mult:
	@(cd bin; make -s mM);

run_boundedBuffer:
	@(cd bin; make -s bB)

run_test1:
	@(cd bin; make -s t1)

run_test2:
	@(cd bin; make -s t2)

run_test3:
	@(cd bin; make -s t3)

run_test4:
	@(cd bin; make -s t4)

run_test5:
	@(cd bin; make -s t5)

run_test6:
	@(cd bin; make -s t6)

run_test7:
	@(cd bin; make -s t7)

clean: 
	@rm obj/myThread.o
	@rm bin/boundedBuffer
	@rm bin/matrixMult
	@rm bin/test1
	@rm bin/test2
	@rm bin/test3
	@rm bin/test4
	@rm bin/test5
	@rm bin/test6
	@rm bin/test7
	@rm lib/libmyThread.so
	@rm output/*
	


		
