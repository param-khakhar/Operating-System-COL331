.PHONY := execute clean

compile:
	@(cd src; $(MAKE))
	@(cd obj; $(MAKE))

execute:
	@(cd bin; $(MAKE))

clean: 
	@rm -f obj/main.o
	@rm -f bin/replace
