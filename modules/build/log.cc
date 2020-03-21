/*
Copyright (c) 2020 TINN by Saverio Castellano. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#include "include/v8.h"

#include <sys/stat.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>

#define TRACE(X,Y) LOG4CXX_TRACE(X, Y);
#define DEBUG(X,Y) LOG4CXX_DEBUG(X, Y);
#define INFO(X,Y) LOG4CXX_INFO(X, Y);
#define WARN(X,Y) LOG4CXX_WARN(X, Y);
#define ERROR(X,Y) LOG4CXX_ERROR(X, Y);
#define FATAL(X,Y) LOG4CXX_FATAL(X, Y);


#include "v8adapt.h"		

#if defined(_WIN32)
  #define LIBRARY_API __declspec(dllexport)
#else
  #define LIBRARY_API
#endif

#include <string>
	

using namespace std;
using namespace v8;


typedef struct {
#ifdef _WIN32
	map<std::string, LoggerPtr> handles;
	LoggerPtr mainLogger;
#else
	map<std::string, log4cxx::LoggerPtr> handles;
	log4cxx::LoggerPtr mainLogger;	
#endif
	
} LogContext;

#ifndef _WIN32 
using namespace log4cxx; 
#endif

static Local<Value> Throw(Isolate* isolate, const char* message) {
	return isolate->ThrowException(v8::Exception::Error(String::NewFromUtf8(isolate, message, NewStringType::kNormal).ToLocalChecked()));
}


LogContext* GetLogContext(Isolate* isolate, Local<v8::Object> object) {
  LogContext* ctx =
  static_cast<LogContext*>(object->GetAlignedPointerFromInternalField(0));
  return ctx;
}

LoggerPtr getLogger(LogContext *ctx, const v8::FunctionCallbackInfo<v8::Value>& args) {
    Isolate* isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);
	
	LoggerPtr logger = ctx->mainLogger;
	if (args.Length() > 1 && args[1]->IsString()) {
		v8::String::Utf8Value jsName(isolate,Handle<v8::String>::Cast(args[1]));
		std::string name = std::string(*jsName);
		if (ctx->handles.find(name)==ctx->handles.end()){
			ctx->handles[name] = log4cxx::Logger::getLogger(name.c_str());
		}	
		logger = ctx->handles[name];
	}
	return logger;
}

static void LogTrace(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);
	if (args.Length() == 0)
	{		
		Throw(isolate,"invalid arguments");
	} else if (!args[0]->IsString())
	{
		Throw(isolate,"invalid string argument");		
	}
	
	LogContext *ctx = GetLogContext(isolate, args.Holder());	
	if (ctx->mainLogger == NULL) {
		Throw(isolate,"Log module not initialized");
		return;
	}
	
	
	v8::String::Utf8Value jsStr(isolate,Handle<v8::String>::Cast(args[0]));
	TRACE(getLogger(ctx, args), *jsStr)
}

static void LogDebug(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);
   
	if (args.Length() == 0)
	{		
		Throw(isolate,"invalid arguments");
	} else if (!args[0]->IsString())
	{
		Throw(isolate,"invalid string argument");		
	}

	LogContext *ctx = GetLogContext(isolate, args.Holder());	
	if (ctx->mainLogger == NULL) {
		Throw(isolate,"Log module not initialized");
		return;
	}
	v8::String::Utf8Value jsStr(isolate,Handle<v8::String>::Cast(args[0]));
	
	DEBUG(getLogger(ctx, args), *jsStr)
}
static void LogInfo(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);
   
	if (args.Length() == 0)
	{		
		Throw(isolate,"invalid arguments");
	} else if (!args[0]->IsString())
	{
		Throw(isolate,"invalid string argument");		
	}
	
	LogContext *ctx = GetLogContext(isolate, args.Holder());	
	if (ctx->mainLogger == NULL) {
		Throw(isolate,"Log module not initialized");
		return;
	}
	
	v8::String::Utf8Value jsStr(isolate,Handle<v8::String>::Cast(args[0]));
	INFO(getLogger(ctx, args), *jsStr)
}
static void LogWarn(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);
   
	if (args.Length() == 0)
	{		
		Throw(isolate,"invalid arguments");
	} else if (!args[0]->IsString())
	{
		Throw(isolate,"invalid string argument");		
	}
	
	LogContext *ctx = GetLogContext(isolate, args.Holder());	
	if (ctx->mainLogger == NULL) {
		Throw(isolate,"Log module not initialized");
		return;
	}
	
	v8::String::Utf8Value jsStr(isolate,Handle<v8::String>::Cast(args[0]));
	WARN(getLogger(ctx, args), *jsStr)
}
static void LogError(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);
   
	if (args.Length() == 0)
	{		
		Throw(isolate,"invalid arguments");
	} else if (!args[0]->IsString())
	{
		Throw(isolate,"invalid string argument");		
	}
	
	LogContext *ctx = GetLogContext(isolate, args.Holder());	
	if (ctx->mainLogger == NULL) {
		Throw(isolate,"Log module not initialized");
		return;
	}
	
	v8::String::Utf8Value jsStr(isolate,Handle<v8::String>::Cast(args[0]));
	ERROR(getLogger(ctx, args), *jsStr)
}
static void LogFatal(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);
   
	if (args.Length() == 0)
	{		
		Throw(isolate,"invalid arguments");
	} else if (!args[0]->IsString())
	{
		Throw(isolate,"invalid string argument");		
	}

	LogContext *ctx = GetLogContext(isolate, args.Holder());	
	if (ctx->mainLogger == NULL) {
		Throw(isolate,"Log module not initialized");
		return;
	}
	
	
	v8::String::Utf8Value jsStr(isolate,Handle<v8::String>::Cast(args[0]));
	FATAL(getLogger(ctx, args), *jsStr)
}


static void LogInit(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);

	if (args.Length() != 1)
	{		
		Throw(isolate,"invalid arguments");
	} else if (!args[0]->IsString())
	{
		Throw(isolate,"invalid string argument");		
	}
	
	LogContext *ctx = GetLogContext(isolate, args.Holder());	
	if (ctx->mainLogger != NULL) {
		Throw(isolate,"Log module already initialized");
		return;
	}
	
	v8::String::Utf8Value jsConfigfile(isolate,Handle<v8::String>::Cast(args[0]));
	char * configFile = *jsConfigfile;
	
	struct stat s;
	if  (!( stat(configFile,&s) == 0 && !(s.st_mode & S_IFDIR)))
	{
		char err[FILENAME_MAX];
		sprintf(err,"log configuration file not found: %s", configFile);
		Throw(isolate, err);
		return;		
	}
	
	log4cxx::PropertyConfigurator::configure(configFile);
	ctx->mainLogger = log4cxx::Logger::getRootLogger();	
}



static void LogSetLevel(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);

	if (args.Length() != 1)
	{		
		Throw(isolate,"invalid arguments");
	} else if (!args[0]->IsInt32())
	{
		Throw(isolate,"invalid level");		
	}
	
	LogContext *ctx = GetLogContext(isolate, args.Holder());	
	if (ctx->mainLogger == NULL) {
		Throw(isolate,"Log module not initialized");
		return;
	}
	
	
	int64_t level = (int64_t)args[0]->Uint32Value(isolate->GetCurrentContext()).FromMaybe(0L);
	LoggerPtr logger = getLogger(ctx, args);
	
	logger->setLevel(log4cxx::Level::toLevel(level));
}

static void LogIsLevel(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);

	if (args.Length() == 0)
	{		
		Throw(isolate,"invalid arguments");
	} else if (!args[0]->IsInt32())
	{
		Throw(isolate,"invalid level");		
	}
	
	LogContext *ctx = GetLogContext(isolate, args.Holder());	
	if (ctx->mainLogger == NULL) {
		Throw(isolate,"Log module not initialized");
		return;
	}
	
	int64_t level = (int64_t)args[0]->Uint32Value(isolate->GetCurrentContext()).FromMaybe(0L);
	
	args.GetReturnValue().Set(v8::Boolean::New(isolate,getLogger(ctx, args)->getLevel()->isGreaterOrEqual(log4cxx::Level::toLevel(level))));
}


extern "C" bool LIBRARY_API attach(Isolate* isolate, v8::Local<v8::Context> &context) 
{
	Handle<ObjectTemplate> log = ObjectTemplate::New(isolate);
	
	log->Set(v8::String::NewFromUtf8(isolate, "fatal")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, LogFatal));
	log->Set(v8::String::NewFromUtf8(isolate, "trace")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, LogTrace));
	log->Set(v8::String::NewFromUtf8(isolate, "error")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, LogError));
	log->Set(v8::String::NewFromUtf8(isolate, "info")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, LogInfo));
	log->Set(v8::String::NewFromUtf8(isolate, "debug")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, LogDebug));
	log->Set(v8::String::NewFromUtf8(isolate, "warn")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, LogWarn));
	
	log->Set(v8::String::NewFromUtf8(isolate, "init")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, LogInit));
	log->Set(v8::String::NewFromUtf8(isolate, "setLevel")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, LogSetLevel));
	log->Set(v8::String::NewFromUtf8(isolate, "isLevel")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, LogIsLevel));

	log->Set(v8::String::NewFromUtf8(isolate,"ALL")TO_LOCAL_CHECKED, v8::Integer::New(isolate, log4cxx::Level::ALL_INT));
	log->Set(v8::String::NewFromUtf8(isolate,"OFF")TO_LOCAL_CHECKED, v8::Integer::New(isolate, log4cxx::Level::OFF_INT));	
	log->Set(v8::String::NewFromUtf8(isolate,"ERROR")TO_LOCAL_CHECKED, v8::Integer::New(isolate, log4cxx::Level::ERROR_INT));
	log->Set(v8::String::NewFromUtf8(isolate,"TRACE")TO_LOCAL_CHECKED, v8::Integer::New(isolate, log4cxx::Level::TRACE_INT));
	log->Set(v8::String::NewFromUtf8(isolate,"FATAL")TO_LOCAL_CHECKED, v8::Integer::New(isolate, log4cxx::Level::FATAL_INT));
	log->Set(v8::String::NewFromUtf8(isolate,"WARN")TO_LOCAL_CHECKED, v8::Integer::New(isolate,  log4cxx::Level::WARN_INT));
	log->Set(v8::String::NewFromUtf8(isolate,"DEBUG")TO_LOCAL_CHECKED, v8::Integer::New(isolate, log4cxx::Level::DEBUG_INT));
	log->Set(v8::String::NewFromUtf8(isolate,"INFO")TO_LOCAL_CHECKED, v8::Integer::New(isolate,  log4cxx::Level::INFO_INT));
		
		
	log->SetInternalFieldCount(1);  
	
	v8::Local<v8::Object> instance = log->NewInstance(context).ToLocalChecked();	
	LogContext *ctx = new LogContext();	
	instance->SetAlignedPointerInInternalField(0, ctx);		
	
	context->Global()->Set(context,v8::String::NewFromUtf8(isolate,"Log")TO_LOCAL_CHECKED, instance).FromJust();
	return true;
	

}

extern "C" bool LIBRARY_API init() 
{
 	return true;
}




