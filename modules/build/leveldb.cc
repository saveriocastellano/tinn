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

#include "v8adapt.h"



using namespace std;
using namespace v8;


typedef struct {
	int dbId;
	map<int,leveldb::DB*> dbs;
	leveldb::WriteOptions writeOptions;
} LevelDBContext;


LevelDBContext * gCtx = NULL;

static Local<Value> Throw(Isolate* isolate, const char* message) {
	return isolate->ThrowException(v8::Exception::Error(String::NewFromUtf8(isolate, message, NewStringType::kNormal).ToLocalChecked()));
}

LevelDBContext* GetContext() {

  if (!gCtx)
  {
	  gCtx = new LevelDBContext();
	  gCtx->dbId = 0; 
	  //object->SetInternalField(0, v8::External::New(isolate,ctx));
  }
  
  return gCtx;
}

static void LevelDBOpen(const v8::FunctionCallbackInfo<v8::Value>& args) {
	Isolate* isolate = args.GetIsolate();
	HandleScope handle_scope(isolate);  
  
	if (args.Length() != 1 || !args[0]->IsString())
	{		
		Throw(isolate,"invalid arguments");
		return;
	}
		
	v8::String::Utf8Value jsPath(isolate,Handle<v8::String>::Cast(args[0]));
	
	leveldb::DB* db;
    leveldb::Options options;
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options, *jsPath, &db);	
    if (false == status.ok())
    {	
		args.GetReturnValue().Set( v8::Integer::New(isolate, -1));
		return;
	}
	LevelDBContext * ctx = GetContext();
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

	LevelDBContext * ctx = GetContext();
	
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

	LevelDBContext * ctx = GetContext();
	
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

	LevelDBContext * ctx = GetContext();
	
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
	LevelDBContext * ctx = GetContext();
	
	if (ctx->dbs.find(dbId)==ctx->dbs.end()){
		Throw(isolate,"invalid db handle");
		return;
	}
	
	delete ctx->dbs[dbId];
	ctx->dbs.erase(dbId);

}

extern "C" void attach(Isolate* isolate, Local<ObjectTemplate> &global_template) 
{
	Handle<ObjectTemplate> leveldb = ObjectTemplate::New(isolate);
	
	leveldb->Set(v8::String::NewFromUtf8(isolate, "open")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, LevelDBOpen));
	leveldb->Set(v8::String::NewFromUtf8(isolate, "put")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, LevelDBPut));
	leveldb->Set(v8::String::NewFromUtf8(isolate, "get")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, LevelDBGet));
	leveldb->Set(v8::String::NewFromUtf8(isolate, "delete")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, LevelDBDelete));
	leveldb->Set(v8::String::NewFromUtf8(isolate, "close")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, LevelDBClose));
	
	global_template->Set(v8::String::NewFromUtf8(isolate,"LevelDB")TO_LOCAL_CHECKED, leveldb);

}

extern "C" bool init() 
{
	return true;
}