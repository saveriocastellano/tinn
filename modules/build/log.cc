/*
Copyright (c) 2020 TINN by Saverio Castellano. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#include "include/v8.h"

#include <sys/stat.h>
#include <unistd.h>

#include <log4cxx/logger.h>
#include <log4cxx/xml/domconfigurator.h>


log4cxx::LevelPtr mainLevel;
log4cxx::LoggerPtr mainLogger(log4cxx::Logger::getLogger( "main"));
#define TRACE(X) LOG4CXX_TRACE(mainLogger, X);
#define DEBUG(X) LOG4CXX_DEBUG(mainLogger, X);
#define INFO(X) LOG4CXX_INFO(mainLogger, X);
#define WARN(X) LOG4CXX_WARN(mainLogger, X);
#define ERROR(X) LOG4CXX_ERROR(mainLogger, X);
#define FATAL(X) LOG4CXX_FATAL(mainLogger, X);


#include "v8adapt.h"		


using namespace std;
using namespace v8;

static bool gInit = false;

static Local<Value> Throw(Isolate* isolate, const char* message) {
	return isolate->ThrowException(v8::Exception::Error(String::NewFromUtf8(isolate, message, NewStringType::kNormal).ToLocalChecked()));
}

static void LogTrace(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);
	if (!gInit) Throw(isolate, "logger not initialized");
	if (args.Length() != 1)
	{		
		Throw(isolate,"invalid arguments");
	} else if (!args[0]->IsString())
	{
		Throw(isolate,"invalid string argument");		
	}
	
	v8::String::Utf8Value jsStr(isolate,Handle<v8::String>::Cast(args[0]));
	TRACE(*jsStr)
}

static void LogDebug(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);
	if (!gInit) Throw(isolate, "logger not initialized");
   
	if (args.Length() != 1)
	{		
		Throw(isolate,"invalid arguments");
	} else if (!args[0]->IsString())
	{
		Throw(isolate,"invalid string argument");		
	}
	
	v8::String::Utf8Value jsStr(isolate,Handle<v8::String>::Cast(args[0]));
	DEBUG(*jsStr)
}
static void LogInfo(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);
	if (!gInit) Throw(isolate, "logger not initialized");
   
	if (args.Length() != 1)
	{		
		Throw(isolate,"invalid arguments");
	} else if (!args[0]->IsString())
	{
		Throw(isolate,"invalid string argument");		
	}
	
	v8::String::Utf8Value jsStr(isolate,Handle<v8::String>::Cast(args[0]));
	INFO(*jsStr)
}
static void LogWarn(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);
	if (!gInit) Throw(isolate, "logger not initialized");
   
	if (args.Length() != 1)
	{		
		Throw(isolate,"invalid arguments");
	} else if (!args[0]->IsString())
	{
		Throw(isolate,"invalid string argument");		
	}
	
	v8::String::Utf8Value jsStr(isolate,Handle<v8::String>::Cast(args[0]));
	WARN(*jsStr)
}
static void LogError(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);
	if (!gInit) Throw(isolate, "logger not initialized");
   
	if (args.Length() != 1)
	{		
		Throw(isolate,"invalid arguments");
	} else if (!args[0]->IsString())
	{
		Throw(isolate,"invalid string argument");		
	}
	
	v8::String::Utf8Value jsStr(isolate,Handle<v8::String>::Cast(args[0]));
	ERROR(*jsStr)
}
static void LogFatal(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);
	if (!gInit) Throw(isolate, "logger not initialized");
   
	if (args.Length() != 1)
	{		
		Throw(isolate,"invalid arguments");
	} else if (!args[0]->IsString())
	{
		Throw(isolate,"invalid string argument");		
	}
	
	v8::String::Utf8Value jsStr(isolate,Handle<v8::String>::Cast(args[0]));
	FATAL(*jsStr)
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
	
	log4cxx::xml::DOMConfigurator::configure(configFile);
	mainLevel = log4cxx::Level::getDebug();
	mainLogger->setLevel(mainLevel);
	
	gInit = true;
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
	
	int64_t level = (int64_t)args[0]->Uint32Value(isolate->GetCurrentContext()).FromMaybe(0L);
	
	mainLevel = log4cxx::Level::toLevel(level);
	mainLogger->setLevel(mainLevel);
}

static void LogIsLevel(const v8::FunctionCallbackInfo<v8::Value>& args)
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
	
	int64_t level = (int64_t)args[0]->Uint32Value(isolate->GetCurrentContext()).FromMaybe(0L);
	
	args.GetReturnValue().Set(v8::Boolean::New(isolate,mainLevel->isGreaterOrEqual(log4cxx::Level::toLevel(level))));
}




extern "C" void attach(Isolate* isolate, Local<ObjectTemplate> &global_template) 
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
	
	log->Set(v8::String::NewFromUtf8(isolate,"ERROR")TO_LOCAL_CHECKED, v8::Integer::New(isolate, log4cxx::Level::ERROR_INT));
	log->Set(v8::String::NewFromUtf8(isolate,"TRACE")TO_LOCAL_CHECKED, v8::Integer::New(isolate, log4cxx::Level::TRACE_INT));
	log->Set(v8::String::NewFromUtf8(isolate,"FATAL")TO_LOCAL_CHECKED, v8::Integer::New(isolate, log4cxx::Level::FATAL_INT));
	log->Set(v8::String::NewFromUtf8(isolate,"WARN")TO_LOCAL_CHECKED, v8::Integer::New(isolate, log4cxx::Level::WARN_INT));
	log->Set(v8::String::NewFromUtf8(isolate,"DEBUG")TO_LOCAL_CHECKED, v8::Integer::New(isolate, log4cxx::Level::DEBUG_INT));
	log->Set(v8::String::NewFromUtf8(isolate,"INFO")TO_LOCAL_CHECKED, v8::Integer::New(isolate, log4cxx::Level::INFO_INT));
		
		
	global_template->Set(v8::String::NewFromUtf8(isolate,"Log")TO_LOCAL_CHECKED, log);

}

extern "C" bool init() 
{
 	return true;
}




