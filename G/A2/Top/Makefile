.PHONY := execute clean

run:
	@(cd bin; $(MAKE))

compile:
	@(cd src; $(MAKE))
	@(cd obj; $(MAKE))

plot:
	@(cd src; make Plot)
	@(cd obj; make Plot)
	@(cd bin; make Plot)

clean: 
	@rm -f obj/main.o
	@rm -f src/main.c
	@rm -f bin/primeChecker
	@rm -f output/*
	@rm -f extras/*
	


		
