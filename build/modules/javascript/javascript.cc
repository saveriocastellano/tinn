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
#include <map>
#include <time.h>

#include "v8adapt.h"

#if defined(_WIN32)
  #define LIBRARY_API __declspec(dllexport)
#else
  #define LIBRARY_API
#endif


using namespace std;
using namespace v8;

typedef struct {
	int compiledTime;
	v8::Persistent<v8::Script, v8::CopyablePersistentTraits<v8::Script>> script;
} ScriptInfo;


typedef struct {
	std::map<std::string, ScriptInfo> scripts;
} JsContext;


/*
std::string Basename(const std::string& filename) {
#ifdef _WIN32
  return filename.substr(0, filename.find_last_of("/\\"));
#else	
  char copy[filename.size() + 1];
  memcpy(copy, filename.data(), filename.size());
  copy[filename.size()] = 0;
  return std::string(basename(copy));
#endif   
}*/

static Local<Value> Throw(Isolate* isolate, const char* message) {
	return isolate->ThrowException(v8::Exception::Error(String::NewFromUtf8(isolate, message, NewStringType::kNormal).ToLocalChecked()));
}

JsContext* GetJsContextFromInternalField(Isolate* isolate, Local<Object> object, bool inited = true) {
  JsContext* ctx =
  static_cast<JsContext*>(object->GetAlignedPointerFromInternalField(0));
  if (ctx == NULL) {
    Throw(isolate, "mod_javascript: ctx is defunct ");
    return NULL;
  }
  return ctx;
}


static FILE* FOpen(const char* path, const char* mode) {
#if defined(_MSC_VER) && (defined(_WIN32) || defined(_WIN64))
  FILE* result;
  if (fopen_s(&result, path, mode) == 0) {
    return result;
  } else {
    return NULL;
  }
#else
  FILE* file = fopen(path, mode);
  if (file == NULL) return NULL;
  struct stat file_stat;
  if (fstat(fileno(file), &file_stat) != 0) return NULL;
  bool is_regular_file = ((file_stat.st_mode & S_IFREG) != 0);
  if (is_regular_file) return file;
  fclose(file);
  return NULL;
#endif
}

static char* ReadChars(const char* name, int* size_out) {
  FILE* file = FOpen(name, "rb");
  if (file == NULL) return NULL;

  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  rewind(file);

  char* chars = new char[size + 1];
  chars[size] = '\0';
  for (size_t i = 0; i < size;) {
    i += fread(&chars[i], 1, size - i, file);
    if (ferror(file)) {
      fclose(file);
      delete[] chars;
      return nullptr;
    }
  }
  fclose(file);
  *size_out = static_cast<int>(size);
  return chars;
}

Local<String> ReadFile(Isolate* isolate, const char* name) {
  int size = 0;
  char* chars = ReadChars(name, &size);
  if (chars == NULL) return Local<String>();
  Local<String> result = String::NewFromUtf8(isolate, chars, NewStringType::kNormal, size).ToLocalChecked();
    delete[] chars;
  
  return result;
}

static void Run(const v8::FunctionCallbackInfo<v8::Value>& args) {

  Isolate* isolate = args.GetIsolate();
  HandleScope handle_scope(isolate);  
  
  Local<Context> context = isolate->GetCurrentContext();

  if (args.Length() < 1 || !args[0]->IsString())
  {		
		Throw(isolate,"invalid arguments");
  } 
  String::Utf8Value file(isolate,args[0]);
    if (*file == NULL) {
      Throw(args.GetIsolate(), "Error loading file");
      return;
    }
  
  JsContext *ctx = GetJsContextFromInternalField(isolate, args.Holder());
  struct stat st; 
  stat(*file, &st);
  Local<v8::Script> compiled_script ;

  if (ctx->scripts.find(string(*file)) != ctx->scripts.end() && st.st_mtime < ctx->scripts[string(*file)].compiledTime) {
		compiled_script = *reinterpret_cast<v8::Local<v8::Script>*>(const_cast<v8::Persistent<v8::Script, v8::CopyablePersistentTraits<v8::Script>>*>(&(ctx->scripts[string(*file)].script)));	  
  } else {
	  if (ctx->scripts.find(string(*file)) != ctx->scripts.end())
	  {
		  //outdated found compiled script 
		  ctx->scripts[string(*file)].script.Reset();
		  ctx->scripts.erase(string(*file));
	  }	  
	  
	  Local<String> source = ReadFile(args.GetIsolate(), *file);
	  if (source.IsEmpty()) {
		  Throw(args.GetIsolate(), "Error loading file");
		  return;
	  }
	  MaybeLocal<Script> maybe_script;
	  
	  if (args.Length() > 1 && !args[1]->IsString()) {
		  Throw(args.GetIsolate(), "invalid name argument");  
		  return;
	  }
	  
	  if (args.Length() > 1) {
		  v8::ScriptOrigin origin(Handle<v8::String>::Cast(args[1]));
		  maybe_script = Script::Compile(isolate->GetCurrentContext(), source, &origin);
		  if (!maybe_script.ToLocal(&compiled_script)) {
			  return;
		  }	  

	  } else{
		  v8::Handle<v8::Value> name = v8::String::NewFromUtf8(isolate, *file)TO_LOCAL_CHECKED;
		  v8::ScriptOrigin origin(name);
		  maybe_script = Script::Compile(isolate->GetCurrentContext(), source, &origin);
		  if (!maybe_script.ToLocal(&compiled_script)) {
			  return;
		  }	  
	  }	  
	  
	  
	  v8::Persistent<v8::Script, v8::CopyablePersistentTraits<v8::Script>> value(isolate, compiled_script);
	  ScriptInfo si;
	  si.script = value;
	  si.compiledTime = time(NULL);
	  ctx->scripts[string(*file)] = si;	  
  }
  
  compiled_script->Run(context);
  
}

void ReportException(Isolate* isolate, v8::TryCatch* try_catch) {
	
}


extern "C" bool LIBRARY_API attach(Isolate* isolate, v8::Local<v8::Context> &context) 
{
	v8::HandleScope handle_scope(isolate);
    Context::Scope scope(context);
	Handle<ObjectTemplate> js = ObjectTemplate::New(isolate);
	js->Set(v8::String::NewFromUtf8(isolate, "load")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, Run));	
	js->SetInternalFieldCount(1);  
	v8::Local<v8::Object> instance = js->NewInstance(context).ToLocalChecked();	
    JsContext * ctx = new JsContext();
	instance->SetAlignedPointerInInternalField(0, ctx);		
	context->Global()->Set(context,v8::String::NewFromUtf8(isolate,"JS")TO_LOCAL_CHECKED, instance).FromJust();
	return true;

}

extern "C" bool LIBRARY_API init() 
{
	return true;
}