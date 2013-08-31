#if lua is not installed yet, then run make luajit before make

all: 
	cd src;	make -j 5;
luajit:
	cd luajit2; make install -j 5;
luajit-clean:
	cd luajit2; make clean;
clean:
	cd src; make clean;

.PHONY: all luajit luajit-clean clean


