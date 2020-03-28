
include ../Makefile

all:
	$(CC) $(CFLAGS) -I$(SSDB_HOME)/src  ../../src/sds.c http.cc -lfcgi++ -lfcgi -lcurl -o ../../../modules/mod_http.so

clean:
	rm ../../../modules/mod_http.so