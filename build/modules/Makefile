DIR := $(shell pwd)
#V8_VERSION=`cat V8-VERSION`
V8_VERSION := $(shell cat ../../V8-VERSION)

#HIREDIS_HOME= ../../libs/hiredis
#SSDB_HOME=$(DIR)/../build/libs/ssdb
#HIREDIS_CPP_CLUSTER_HOME=$(DIR)/../build/libs/hiredis/cpp-hiredis-cluster
#V8_VERSION=7.9.2
V8_HOME=../../includes/v8_$(V8_VERSION)
#LIBSIGAR=-lsigar-amd64-linux
CFLAGS=-O3 -std=c++11 -fPIC -I$(V8_HOME) -I../../includes -w -fpermissive -shared 
CC=g++
 
