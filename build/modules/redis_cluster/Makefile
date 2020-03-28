
HIREDIS_HOME=platform/linux/hiredis
CPP_HIREDIS_CLUSTER_HOME=platform/linux/cpp-hiredis-cluster

include ../Makefile

all: hiredis
	$(CC) $(CFLAGS) -I$(HIREDIS_HOME) -I$(HIREDIS_HOME)/.. -I$(CPP_HIREDIS_CLUSTER_HOME)/include ../../src/sds.c redis_cluster.cc $(HIREDIS_HOME)/libhiredis.a -o ../../../modules/mod_redis_cluster.so
	
hiredis:
	$(MAKE) -C $(HIREDIS_HOME)
	
clean:
	rm ../../../modules/mod_redis_cluster.so
	$(MAKE) -C $(HIREDIS_HOME) clean