/*
Copyright (c) 2020 TINN by Saverio Castellano. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#include "include/v8.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <map>

#include "leveldb/db.h"
#include "leveldb/cache.h"
#include "leveldb/filter_policy.h"

#include "v8adapt.h"

#if defined(_WIN32)
  #define LIBRARY_API __declspec(dllexport)
#else
  #define LIBRARY_API
#endif


using namespace std;
using namespace v8;




typedef struct {
	int dbId;
	map<int,leveldb::DB*> dbs;
	leveldb::WriteOptions writeOptions;
} LevelDBContext;


static Local<Value> Throw(Isolate* isolate, const char* message) {
	return isolate->ThrowException(v8::Exception::Error(String::NewFromUtf8(isolate, message, NewStringType::kNormal).ToLocalChecked()));
}


LevelDBContext* GetContext(Isolate* isolate, Local<v8::Object> object) {
  LevelDBContext* ctx =
  static_cast<LevelDBContext*>(object->GetAlignedPointerFromInternalField(0));
  return ctx;
}


static void LevelDBOpen(const v8::FunctionCallbackInfo<v8::Value>& args) {
	Isolate* isolate = args.GetIsolate();
	HandleScope handle_scope(isolate);  
    Local<Context> context = isolate->GetCurrentContext();
	
	if (args.Length() < 1 || !args[0]->IsString())
	{		
		Throw(isolate,"invalid arguments");
		return;
	}
		
	v8::String::Utf8Value jsPath(isolate,Handle<v8::String>::Cast(args[0]));
	
	leveldb::DB* db;
    leveldb::Options options;
	options.create_if_missing = true;
	
	if (args.Length() > 1) {
		
		if (!args[1]->IsObject()){
			Throw(isolate, "invalid 'options' value");
			return;
		}
	
		Handle<Object> optionsObj = Handle<Object>::Cast(args[1]);	
		Local<Array> props = optionsObj->GetOwnPropertyNames(context).ToLocalChecked();
		for (unsigned int i=0; i<props->Length();i++)
		{
			v8::String::Utf8Value propName(isolate,Handle<v8::String>::Cast(props->Get(CONTEXT_ARG i)TO_LOCAL_CHECKED));
			Local<Value> propVal = optionsObj->Get(context, Handle<v8::String>::Cast(props->Get(CONTEXT_ARG i)TO_LOCAL_CHECKED)).ToLocalChecked();
			
			if (strcmp(*propName,"create_if_missing")==0) {
				if (!propVal->IsBoolean()) {Throw(isolate, "invalid value for 'create_if_missing'"); return;}
				options.create_if_missing = propVal->BooleanValue(isolate); 
			} 	else if (strcmp(*propName,"error_if_exists")==0) {
				if (!propVal->IsBoolean()) {Throw(isolate, "invalid value for 'error_if_exists'"); return;}
				options.error_if_exists = propVal->BooleanValue(isolate); 
			} 	else if (strcmp(*propName,"paranoid_checks")==0) {
				if (!propVal->IsBoolean()) {Throw(isolate, "invalid value for 'paranoid_checks'"); return;}
				options.paranoid_checks = propVal->BooleanValue(isolate); 
			} 	else if (strcmp(*propName,"write_buffer_size")==0) {
				if (!propVal->IsUint32()) {Throw(isolate, "invalid value for 'write_buffer_size'"); return;}
				options.write_buffer_size = propVal->Uint32Value(context).FromMaybe(1L);
			}	else if (strcmp(*propName,"max_open_files")==0) {
				if (!propVal->IsUint32()) {Throw(isolate, "invalid value for 'max_open_files'"); return;}
				options.max_open_files = propVal->Uint32Value(context).FromMaybe(1L);
			}	else if (strcmp(*propName,"block_size")==0) {
				if (!propVal->IsUint32()) {Throw(isolate, "invalid value for 'block_size'"); return;}
				options.block_size = propVal->Uint32Value(context).FromMaybe(1L);
			}	else if (strcmp(*propName,"block_restart_interval")==0) {
				if (!propVal->IsUint32()) {Throw(isolate, "invalid value for 'block_restart_interval'"); return;}
				options.block_restart_interval = propVal->Uint32Value(context).FromMaybe(1L);
			}	else if (strcmp(*propName,"bloom_filter_policy_bits")==0) {
				if (!propVal->IsUint32()) {Throw(isolate, "invalid value for 'bloom_filter_policy_bits'"); return;}
				options.filter_policy  = leveldb::NewBloomFilterPolicy(propVal->Uint32Value(context).FromMaybe(1L));
			}	else if (strcmp(*propName,"compression")==0) {
				if (!propVal->IsBoolean()) {Throw(isolate, "invalid value for 'compression'"); return;}
				if (propVal->BooleanValue(isolate)) {
					options.compression = leveldb::kSnappyCompression;
				} else {
					options.compression = leveldb::kNoCompression;
				}
			}	else if (strcmp(*propName,"block_cache_size")==0) {
				if (!propVal->IsUint32()) {Throw(isolate, "invalid value for 'cache_size'"); return;}
				options.block_cache = leveldb::NewLRUCache(propVal->Uint32Value(context).FromMaybe(1L));
			} else {				
				Throw(isolate, "invalid option given");
				return;
			}		
		}	
	}
	
    leveldb::Status status = leveldb::DB::Open(options, *jsPath, &db);	
    if (false == status.ok())
    {	
		printf("Error opening database in %s: %s\n", *jsPath, status.ToString().c_str());
		args.GetReturnValue().Set( v8::Integer::New(isolate, -1));
		return;
	}
	LevelDBContext * ctx = GetContext(isolate, args.Holder());
	ctx->dbId++;
	ctx->dbs[ctx->dbId] = db;
	args.GetReturnValue().Set( v8::Integer::New(isolate, ctx->dbId));
}

static void LevelDBPut(const v8::FunctionCallbackInfo<v8::Value>& args) {
	Isolate* isolate = args.GetIsolate();
	HandleScope handle_scope(isolate);  
  
	if (args.Length() != 3 || !args[0]->IsUint32() || !args[1]->IsString() || !args[2]->IsString())
	{		
		Throw(isolate,"invalid arguments");
		return;
	}

	unsigned int dbId = args[0]->Uint32Value(isolate->GetCurrentContext()).FromMaybe(0L);
	v8::String::Utf8Value jsKey(isolate,Handle<v8::String>::Cast(args[1]));
	v8::String::Utf8Value jsValue(isolate,Handle<v8::String>::Cast(args[2]));

	LevelDBContext * ctx = GetContext(isolate, args.Holder());
	
	if (ctx->dbs.find(dbId)==ctx->dbs.end()){
		Throw(isolate,"invalid db handle");
		return;
	}
	
	leveldb::Status status = ctx->dbs[dbId]->Put(ctx->writeOptions, *jsKey, *jsValue);
	
}

static void LevelDBGet(const v8::FunctionCallbackInfo<v8::Value>& args) {
	Isolate* isolate = args.GetIsolate();
	HandleScope handle_scope(isolate);  
  
	if (args.Length() != 2 || !args[0]->IsUint32() || !args[1]->IsString())
	{		
		Throw(isolate,"invalid arguments");
		return;
	}

	unsigned int dbId = args[0]->Uint32Value(isolate->GetCurrentContext()).FromMaybe(0L);
	v8::String::Utf8Value jsKey(isolate,Handle<v8::String>::Cast(args[1]));

	LevelDBContext * ctx = GetContext(isolate, args.Holder());
	
	if (ctx->dbs.find(dbId)==ctx->dbs.end()){
		Throw(isolate,"invalid db handle");
		return;
	}
	

	std::string document;
	leveldb::Status status = ctx->dbs[dbId]->Get(leveldb::ReadOptions(), *jsKey, &document);
	if (false == status.ok()) {
		return;
	}
	
	Handle<v8::String> jsVal = v8::String::NewFromUtf8(isolate, document.c_str())TO_LOCAL_CHECKED;
	args.GetReturnValue().Set(jsVal);	  
}

static void LevelDBDelete(const v8::FunctionCallbackInfo<v8::Value>& args) {
	Isolate* isolate = args.GetIsolate();
	HandleScope handle_scope(isolate);  
  
	if (args.Length() != 2 || !args[0]->IsUint32() || !args[1]->IsString())
	{		
		Throw(isolate,"invalid arguments");
		return;
	}

	unsigned int dbId = args[0]->Uint32Value(isolate->GetCurrentContext()).FromMaybe(0L);
	v8::String::Utf8Value jsKey(isolate,Handle<v8::String>::Cast(args[1]));

	LevelDBContext * ctx = GetContext(isolate, args.Holder());
	
	if (ctx->dbs.find(dbId)==ctx->dbs.end()){
		Throw(isolate,"invalid db handle");
		return;
	}
	
	ctx->dbs[dbId]->Delete(leveldb::WriteOptions(), *jsKey);

}



static void LevelDBClose(const v8::FunctionCallbackInfo<v8::Value>& args) {
	Isolate* isolate = args.GetIsolate();
	HandleScope handle_scope(isolate);  
  
	if (args.Length() != 1 || !args[0]->IsUint32())
	{		
		printf("invalid args\n");
		Throw(isolate,"invalid arguments");
		return;
	}

	unsigned int dbId = args[0]->Uint32Value(isolate->GetCurrentContext()).FromMaybe(0L);
	LevelDBContext * ctx = GetContext(isolate, args.Holder());
	
	if (ctx->dbs.find(dbId)==ctx->dbs.end()){
		Throw(isolate,"invalid db handle");
		return;
	}
	
	delete ctx->dbs[dbId];
	ctx->dbs.erase(dbId);

}

extern "C" bool LIBRARY_API attach(Isolate* isolate, v8::Local<v8::Context> &context) 
{
	Handle<ObjectTemplate> leveldb = ObjectTemplate::New(isolate);
	
	leveldb->Set(v8::String::NewFromUtf8(isolate, "open")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, LevelDBOpen));
	leveldb->Set(v8::String::NewFromUtf8(isolate, "put")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, LevelDBPut));
	leveldb->Set(v8::String::NewFromUtf8(isolate, "get")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, LevelDBGet));
	leveldb->Set(v8::String::NewFromUtf8(isolate, "delete")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, LevelDBDelete));
	leveldb->Set(v8::String::NewFromUtf8(isolate, "close")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, LevelDBClose));
	
	leveldb->SetInternalFieldCount(1);  
	
	v8::Local<v8::Object> instance = leveldb->NewInstance(context).ToLocalChecked();	
	LevelDBContext *ctx = new LevelDBContext();	
	ctx->dbId = 0;
	instance->SetAlignedPointerInInternalField(0, ctx);		
	
	context->Global()->Set(context,v8::String::NewFromUtf8(isolate,"LevelDB")TO_LOCAL_CHECKED, instance).FromJust();
	return true;
}

extern "C" bool LIBRARY_API init() 
{
	return true;
}