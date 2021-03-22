LDFLAGS="-Wl,-rpath,../lib/"

Re: 
	@gcc -Wall -fpic -c myThread.c
	@mv myThread.o ../obj
	@(cd ..; cd lib; $(MAKE))

bB:
	@gcc -Wall -fpic -c myThread.c
	@mv myThread.o ../obj
	@(cd ..; cd lib; $(MAKE))
	@gcc $(LDFLAGS) -L../lib/ boundedBuffer.c -lmyThread -o boundedBuffer
	@mv boundedBuffer ../bin

mM:
	@gcc -Wall -fpic -c myThread.c
	@mv myThread.o ../obj
	@(cd ..; cd lib; $(MAKE))
	@gcc $(LDFLAGS) -L../lib/ matrixMult.c -lmyThread -o matrixMult -w -lpthread
	@mv matrixMult ../bin

tests:
	@gcc -Wall -fpic -c myThread.c
	@mv myThread.o ../obj
	@(cd ..; cd lib; $(MAKE))
	@gcc $(LDFLAGS) -L../lib/ test1.c -lmyThread -o test1 -w
	@mv test1 ../bin
	@gcc $(LDFLAGS) -L../lib/ test2.c -lmyThread -o test2 -w
	@mv test2 ../bin
	@gcc $(LDFLAGS) -L../lib/ test3.c -lmyThread -o test3 -w
	@mv test3 ../bin
	@gcc $(LDFLAGS) -L../lib/ test4.c -lmyThread -o test4 -w
	@mv test4 ../bin
	@gcc $(LDFLAGS) -L../lib/ test5.c -lmyThread -o test5 -w
	@mv test5 ../bin
	@gcc $(LDFLAGS) -L../lib/ test6.c -lmyThread -o test6 -w
	@mv test6 ../bin
	@gcc $(LDFLAGS) -L../lib/ test7.c -lmyThread -o test7 -w
	@mv test7 ../bin