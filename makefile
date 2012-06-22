nearest-module:	nearest-module.c nearest-module.h
	cc -g -Wall -O -o $@ -DTEST nearest-module.c -lz

nearest-module.h:	nearest-module.c
	cfunctions -inc nearest-module.c

clean:
	-rm -f nearest-module
