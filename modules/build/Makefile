DIR=`pwd`
HIREDIS_HOME=$(DIR)/../build/libs/hiredis
SSDB_HOME=$(DIR)/../build/libs/ssdb
HIREDIS_CPP_CLUSTER_HOME=$(DIR)/../build/libs/hiredis/cpp-hiredis-cluster
V8_VERSION=7.9
V8_HOME=$(DIR)/libs/v8_$(V8_VERSION)
LIBSIGAR=-lsigar-amd64-linux
CFLAGS=-O3 -std=c++11 -fPIC -I$(V8_HOME) -I$(DIR)/libs -I$(DIR)/includes -w -fpermissive -shared 
CC=g++
 
all: mod_http mod_hiredis mod_os mod_log mod_ssdb mod_hyperloglog mod_leveldb mod_javascript mod_hiredis_cluster 
	
hiredis:
	$(MAKE) -C $(HIREDIS_HOME)
	
mod_javascript: javascript.cc
	$(CC) $(CFLAGS) javascript.cc -o ../mod_javascript.so

mod_leveldb: leveldb.cc
	$(CC) $(CFLAGS) leveldb.cc -o ../mod_leveldb.so -lleveldb

mod_hyperloglog: hyperloglog.cc
	$(CC) $(CFLAGS) -I$(DIR)/../build/libs/cpp-HyperLogLog/include hyperloglog.cc -o ../mod_hyperloglog.so

mod_log: log.cc
	$(CC) $(CFLAGS) log.cc -o ../mod_log.so -llog4cxx 
	
mod_os: os.cc
	$(CC) $(CFLAGS) os.cc -o ../mod_os.so

mod_http: http.cc
	$(CC) $(CFLAGS) http.cc $(DIR)/libs/sds.c -o ../mod_http.so -lfcgi++ -lfcgi -lcurl 
	
mod_hiredis: hiredis hiredis.cc 
	$(CC) $(CFLAGS) -I$(HIREDIS_HOME) -I$(DIR)/libs -I $(HIREDIS_CPP_CLUSTER_HOME)/include -L$(HIREDIS_HOME) -I$(HIREDIS_HOME)/redis-client hiredis.cc $(DIR)/libs/sds.c $(HIREDIS_HOME)/libhiredis.a -o ../mod_hiredis.so

mod_hiredis_cluster: hiredis hiredis_cluster.cc
	$(CC) $(CFLAGS) -I$(HIREDIS_HOME) -I$(DIR)/libs -I $(HIREDIS_CPP_CLUSTER_HOME)/include -L$(HIREDIS_HOME) hiredis_cluster.cc $(HIREDIS_HOME)/libhiredis.a -o ../mod_hiredis_cluster.so -levent 

mod_ssdb: ssdb.cc 
	$(CC) $(CFLAGS) -I$(HIREDIS_HOME) -I$(SSDB_HOME)/src -I$(SSDB_HOME)/src/client -I$(DIR)/libs $(DIR)/libs/sds.c ssdb.cc $(SSDB_HOME)/src/util/bytes.cpp $(SSDB_HOME)/src/net/link.cpp $(SSDB_HOME)/src/client/SSDB_impl.cpp -o ../mod_ssdb.so

clean:
	rm -rf ../*.so 
	$(MAKE) -C $(HIREDIS_HOME) clean

