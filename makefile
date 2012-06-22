nearest-module:	nearest-module.c
	cc -g -Wall -O -o $@ nearest-module.c -lz

clean:
	-rm -f nearest-module
