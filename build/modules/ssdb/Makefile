SSDB_HOME=./lib

include ../Makefile

all:
	$(CC) $(CFLAGS) -I$(SSDB_HOME)/src -I$(SSDB_HOME)/src/client ../../src/sds.c ssdb.cc $(SSDB_HOME)/src/util/bytes.cpp \
	$(SSDB_HOME)/src/net/link.cpp $(SSDB_HOME)/src/client/SSDB_impl.cpp -o ../../../modules/mod_ssdb.so
	
clean:
	rm ../../../modules/mod_ssdb.so