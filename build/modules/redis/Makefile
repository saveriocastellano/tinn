
HIREDIS_HOME=linux/hiredis

include ../Makefile

all: hiredis
	$(CC) $(CFLAGS) -I$(HIREDIS_HOME)  ../../src/sds.c redis.cc $(HIREDIS_HOME)/libhiredis.a -o ../../../modules/mod_redis.so
	
hiredis:	
	$(MAKE) -C $(HIREDIS_HOME)
	
clean:
	rm ../../../modules/mod_redis.so
	$(MAKE) -C $(HIREDIS_HOME) clean