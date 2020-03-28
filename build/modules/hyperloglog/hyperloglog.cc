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
#include "hyperloglog.hpp"

#include "v8adapt.h"

	
#if defined(_WIN32)
  #define LIBRARY_API __declspec(dllexport)
#else
  #define LIBRARY_API
#endif

using namespace std;
using namespace v8;
using namespace hll;

static Local<Value> Throw(Isolate* isolate, const char* message) {
	return isolate->ThrowException(v8::Exception::Error(String::NewFromUtf8(isolate, message, NewStringType::kNormal).ToLocalChecked()));
}

Handle<Array> dumpHll(Isolate* isolate, HyperLogLog &hll) {
    
    Local<Context> context = isolate->GetCurrentContext();
    Context::Scope context_scope(context);	
	
	stringstream ss(stringstream::in | stringstream::out |stringstream::binary);
	hll.dump(ss);

    std::streambuf * pbuf = ss.rdbuf();
    std::streamsize size = pbuf->pubseekoff(0,ss.end);
    pbuf->pubseekoff(0,ss.beg);       // rewind
    char* contents = new char [size];
	pbuf->sgetn (contents,size);
	
	Handle<Array> jsBytes = v8::Array::New(isolate, size);
	for (unsigned int i=0; i<size; i++)
	{
		jsBytes->Set(CONTEXT_ARG i, v8::Integer::New(isolate, contents[i]));
	}
	delete[] contents;
	return jsBytes;
}

void restoreHll(Isolate * isolate, Handle<Array> aBytes, HyperLogLog &hll) {
    
    Local<Context> context = isolate->GetCurrentContext();
    Context::Scope context_scope(context);	
	
	std::streamsize size = aBytes->Length();    
	char* contents = new char [size];
	stringstream ss2(stringstream::in | stringstream::out |stringstream::binary);
	for (unsigned int i=0; i<size;i++)
	{
		contents[i]= aBytes->Get(CONTEXT_ARG i)TO_LOCAL_CHECKED ->Uint32Value(isolate->GetCurrentContext()).FromMaybe(0L);
	}
	ss2.write(contents,size);
	hll.restore(ss2);
	delete[] contents;	
}


static void HyperLogLogAddToNew(const v8::FunctionCallbackInfo<v8::Value>& args) {
	Isolate* isolate = args.GetIsolate();
	HandleScope handle_scope(isolate);  
  
	if (args.Length() != 2)
	{		
		Throw(isolate,"invalid arguments");
		return;
	} else if (!args[1]->IsString() || !args[0]->IsUint32())
	{
		Throw(isolate,"invalid arguments");
		return;
	}

	unsigned int b = args[0]->Uint32Value(isolate->GetCurrentContext()).FromMaybe(0L);
	v8::String::Utf8Value jsStr(isolate,Handle<v8::String>::Cast(args[1]));
	char * str = *jsStr;
	
	HyperLogLog hll(b);
	hll.add(str, strlen(str));
	
	args.GetReturnValue().Set( dumpHll(isolate, hll));	
}



static void HyperLogLogAdd(const v8::FunctionCallbackInfo<v8::Value>& args) {
	Isolate* isolate = args.GetIsolate();
	HandleScope handle_scope(isolate);  
  
	if (args.Length() != 2)
	{		
		Throw(isolate,"invalid arguments");
		return;
	} else if (!args[1]->IsString() || !args[0]->IsArray())
	{
		Throw(isolate,"invalid string arguments");
		return;
	}

	v8::String::Utf8Value jsAdd(isolate,Handle<v8::String>::Cast(args[1]));
	char * add = *jsAdd;

	Handle<Array> aBytes = Handle<Array>::Cast(args[0]);
	
	HyperLogLog hll2;
	restoreHll(isolate,aBytes, hll2);	
	hll2.add(add, strlen(add));
	
	args.GetReturnValue().Set( dumpHll(isolate, hll2));	
}


static void HyperLogLogMerge(const v8::FunctionCallbackInfo<v8::Value>& args) {
	Isolate* isolate = args.GetIsolate();
	HandleScope handle_scope(isolate);  
  
	if (args.Length() != 2)
	{		
		Throw(isolate,"invalid arguments");
		return;
	} else if (!args[0]->IsArray() || !args[1]->IsArray())
	{
		Throw(isolate,"invalid arguments");
		return;
	}
	Handle<Array> aBytes1 = Handle<Array>::Cast(args[0]);
	Handle<Array> aBytes2 = Handle<Array>::Cast(args[1]);
	

	HyperLogLog hll1;
	restoreHll(isolate,aBytes1, hll1);	

	HyperLogLog hll2;
	restoreHll(isolate,aBytes2, hll2);	
	
	hll1.merge(hll2);
	
	args.GetReturnValue().Set(dumpHll(isolate, hll1));	 
	
}



static void HyperLogLogGetCardinality(const v8::FunctionCallbackInfo<v8::Value>& args) {
	Isolate* isolate = args.GetIsolate();
	HandleScope handle_scope(isolate);  
  
	if (args.Length() != 1)
	{		
		Throw(isolate,"invalid arguments");
		return;
	} else if (!args[0]->IsArray())
	{
		Throw(isolate,"invalid argument");
		return;
	}

	Handle<Array> aBytes = Handle<Array>::Cast(args[0]);
	
	HyperLogLog hll2;
	restoreHll(isolate,aBytes, hll2);	
	
	double cardinality = hll2.estimate();
	args.GetReturnValue().Set( v8::Integer::New(isolate, (int)cardinality));	
}

extern "C" bool LIBRARY_API attach(Isolate* isolate, v8::Local<v8::Context> &context) 
{
	Handle<ObjectTemplate> hll = ObjectTemplate::New(isolate);
	
	hll->Set(v8::String::NewFromUtf8(isolate, "addToNew")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, HyperLogLogAddToNew));
	hll->Set(v8::String::NewFromUtf8(isolate, "add")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, HyperLogLogAdd));
	hll->Set(v8::String::NewFromUtf8(isolate, "getCardinality")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, HyperLogLogGetCardinality));
	hll->Set(v8::String::NewFromUtf8(isolate, "merge")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, HyperLogLogMerge));
	
	v8::Local<v8::Object> instance = hll->NewInstance(context).ToLocalChecked();	
	context->Global()->Set(context,v8::String::NewFromUtf8(isolate,"HyperLogLog")TO_LOCAL_CHECKED, instance).FromJust();	
	
	return true;
}

extern "C" bool LIBRARY_API init() 
{
	return true;
}
