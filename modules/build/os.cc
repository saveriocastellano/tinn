/*
Copyright (c) 2020 TINN by Saverio Castellano. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#include "include/v8.h"
#include <sys/types.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <io.h>
#include <chrono>
#include <thread>   
#else
#include <unistd.h>	
#endif
#include <string.h>
#include <stdlib.h>
#include <sstream>
#ifdef _WIN32
#include <linux.h>
#endif 
#include <dirent.h>
#include <vector>
#include <string>
#include <stdio.h>
#include <errno.h>
#include <map>

#include "v8adapt.h"	
	 
#if defined(_WIN32)
  #define LIBRARY_API __declspec(dllexport)
#else
  #define LIBRARY_API
#endif


typedef unsigned char BYTE;

using namespace std;
using namespace v8;


typedef struct {
	map<unsigned int, FILE*> handles;
	int num;
} OSContext;


static Local<Value> Throw(Isolate* isolate, const char* message) {
	return isolate->ThrowException(v8::Exception::Error(String::NewFromUtf8(isolate, message, NewStringType::kNormal).ToLocalChecked()));
}


OSContext* GetOSContext(Isolate* isolate, Local<Object> object) {
  OSContext* ctx =
  static_cast<OSContext*>(object->GetAlignedPointerFromInternalField(0));
  if (ctx == NULL) {
    Throw(isolate, "ctx is defunct ");
    return NULL;
  }
  return ctx;
}


	
int indexOf_shift (const char* base, const char* str, int startIndex) {
    int result;
    int baselen = (int)strlen(base);
    // str should not longer than base
    if (strlen(str) > baselen || startIndex > baselen) {
        result = -1;
    } else {
        if (startIndex < 0 ) {
            startIndex = 0;
        }
        const char* pos = strstr(base+startIndex, str);
        if (pos == NULL) {
            result = -1;
        } else {
            result = (int)(pos - base);
        }
    }
    return result;
}

int lastIndexOf (const char* base, const char* str) {
    int result;
    // str should not longer than base
    if (strlen(str) > strlen(base)) {
        result = -1;
    } else {
        int start = 0;
        int endinit = (int)(strlen(base) - strlen(str));
        int end = endinit;
        int endtmp = endinit;
        while(start != end) {
            start = indexOf_shift(base, str, start);
            end = indexOf_shift(base, str, end);

            // not found from start
            if (start == -1) {
                end = -1; // then break;
            } else if (end == -1) {
                // found from start
                // but not found from end
                // move end to middle
                if (endtmp == (start+1)) {
                    end = start; // then break;
                } else {
                    end = endtmp - (endtmp - start) / 2;
                    if (end <= start) {
                        end = start+1;
                    }
                    endtmp = end;
                }
            } else {
                // found from both start and end
                // move start to end and
                // move end to base - strlen(str)
                start = end;
                end = endinit;
            }
        }
        result = start;
    }
    return result;
}

int isFileReadable(char * file)
{
	struct stat s;
	return ( stat(file,&s) == 0 && !(s.st_mode & S_IFDIR));
}

int isDirReadable(char * file)
{
	struct stat s;
	return ( stat(file,&s) == 0 && (s.st_mode & S_IFDIR));
}

static void OSSleep(const v8::FunctionCallbackInfo<v8::Value>& args)
{
  Isolate* isolate = args.GetIsolate();
  HandleScope handle_scope(isolate);

	if (args.Length() != 1 || !args[0]->IsUint32())
	{		
		Throw(isolate, "invalid arguments");
		return;
	}
#ifdef _WIN32
	std::this_thread::sleep_for(std::chrono::microseconds(1000*args[0]->Uint32Value(isolate->GetCurrentContext()).FromMaybe(1L))); 	
#else 	
	usleep(1000*args[0]->Uint32Value(isolate->GetCurrentContext()).FromMaybe(1L));
#endif
	return ;
}


static void OSFileSize(const v8::FunctionCallbackInfo<v8::Value>& args) {
    Isolate* isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);
	
	if (args.Length() == 0)
	{		
		Throw(isolate, "no argument");
		return;
	}
	if (!args[0]->IsString())
	{
		Throw(isolate, "invalid argument");				
		return;
	}
	
	v8::String::Utf8Value jsName(isolate,Handle<v8::String>::Cast(args[0]));
	
	struct stat st;
	int res = stat(*jsName, &st);
	if (res<0) {
		args.GetReturnValue().Set(v8::Integer::New(isolate, -1));	
		return;
	}
	int size = st.st_size;	
	args.GetReturnValue().Set(v8::Integer::New(isolate, size));	
	
}



static void OSSetEnv(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope handle_scope(isolate);
  
  if (args.Length() != 2 || !args[0]->IsString() || !args[1]->IsString())
  {
		Throw (isolate,"invalid arguments");
		return;	
  }

  v8::String::Utf8Value jsName(isolate,Handle<v8::String>::Cast(args[0]));
  v8::String::Utf8Value jsVal(isolate,Handle<v8::String>::Cast(args[1]));
  setenv(*jsName, *jsVal, 1);

}




static void OSGetEnv(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope handle_scope(isolate);
  
  if (args.Length() == 0 || !args[0]->IsString())
  {
		Throw (isolate,"invalid arguments");
		return;	
  }

  v8::String::Utf8Value jsName(isolate,Handle<v8::String>::Cast(args[0]));
  char *val = (char*)getenv(*jsName);
  if (!val && args.Length() > 1 && args[1]->IsString())
  {
	  args.GetReturnValue().Set(args[1]);	  
	  return;
  } else if (val) {
	Handle<v8::String> jsVal = v8::String::NewFromUtf8(isolate, val)TO_LOCAL_CHECKED;
	args.GetReturnValue().Set(jsVal);
  }
}



void OSExec(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    Isolate* isolate = args.GetIsolate();
  
  
    HandleScope outer_scope(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    Context::Scope context_scope(context);  
  
	if (args.Length() == 0)
	{		
		Throw(isolate, "no argument");
		return;
	}
	if (!args[0]->IsArray())
	{
		Throw(isolate, "invalid argument");				
		return;
	}
	
	Local<Object> inst = Object::New(isolate);	
	Handle<Array> execArgs = Handle<Array>::Cast(args[0]);

	stringstream ss;
	for (unsigned int i=0; i<execArgs->Length();i++)
	{
		v8::String::Utf8Value jsArg(isolate,Handle<v8::String>::Cast(execArgs->Get(CONTEXT_ARG i)TO_LOCAL_CHECKED));
		ss << std::string(*jsArg) << std::string(" ");
	}
	
	bool close = false;
	if (args.Length() > 1 && !args[1]->IsBoolean()) {
		Throw(isolate, "invalid argument");				
		return;		
	} else if(args.Length() > 1 && args[1]->IsBoolean()){
		close = args[0]->BooleanValue(isolate);
	}

    FILE* pipe = popen(ss.str().c_str(), "r");
    if (!pipe)
	{
		return; 
	} else {	
		if (close) {
			inst->Set(CONTEXT_ARG v8::String::NewFromUtf8(isolate, "result")TO_LOCAL_CHECKED, v8::Integer::New(isolate,pclose(pipe)));		
		} else {
			char buffer[128];
			stringstream res;

			while(!feof(pipe)) {
				if(fgets(buffer, 128, pipe) != NULL)
					res << std::string(buffer);
			}
			inst->Set(CONTEXT_ARG v8::String::NewFromUtf8(isolate, "result")TO_LOCAL_CHECKED, v8::Integer::New(isolate,pclose(pipe)));		
			inst->Set(CONTEXT_ARG v8::String::NewFromUtf8(isolate, "output")TO_LOCAL_CHECKED, v8::String::NewFromUtf8(isolate, res.str().c_str())TO_LOCAL_CHECKED);	
		}
	}
	args.GetReturnValue().Set(inst);	
}


static char* ReadChars(Isolate* isolate, const char* name, int* size_out) {
  FILE* file = fopen(name, "rb");
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

// Reads a file into a v8 string.
Local<String> ReadFile(Isolate* isolate, const char* name) {
  int size = 0;
  char* chars = ReadChars(isolate, name, &size);
  if (chars == NULL) return Local<String>();
  Local<String> result =
      String::NewFromUtf8(isolate, chars, NewStringType::kNormal, size)
          .ToLocalChecked();
  delete[] chars;
  return result;
}

static void OSReadFile(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);
	
	if (args.Length() == 0)
	{		
		Throw(isolate, "no argument");
		return;
	}
	if (!args[0]->IsString())
	{
		Throw(isolate, "invalid argument");				
		return;
	}
	
	v8::String::Utf8Value jsName(isolate,Handle<v8::String>::Cast(args[0]));
	Local<String> content = ReadFile(isolate, *jsName);
	
	args.GetReturnValue().Set(content);	
}

static void OSWriteFile(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);
	
	if (args.Length() != 2)
	{		
		Throw(isolate, "invalid arguments");
		return;
	}
	if (!args[0]->IsString() || !args[1]->IsString())
	{
		Throw(isolate, "invalid arguments");				
		return;
	}
	
	v8::String::Utf8Value jsName(isolate,Handle<v8::String>::Cast(args[0]));
	v8::String::Utf8Value jsContent(isolate,Handle<v8::String>::Cast(args[1]));
	char * buf = *jsContent;
	FILE * f = fopen(*jsName, "wb");
	if (!f)
	{
		args.GetReturnValue().Set(v8::Boolean::New(isolate,false));
		return;
	}
	fwrite(buf , 1 , strlen(buf) , f);
	fclose(f);
	args.GetReturnValue().Set(v8::Boolean::New(isolate,true));		
	
}

static void OSListDir(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);
    Local<Context> context = Context::New(isolate);
    Context::Scope context_scope(context);	
	
	if (args.Length() == 0)
	{		
		Throw(isolate, "invalid arguments");
		return;
	}
	if (!args[0]->IsString() || (args.Length() > 1 && !args[1]->IsString()))
	{
		Throw(isolate, "invalid arguments");				
		return;
	}
	
	v8::String::Utf8Value jsDir(isolate,Handle<v8::String>::Cast(args[0]));
	
	DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(*jsDir)) == NULL) {	
		Throw(isolate, "error opening directory");
		return;
	}
	char *ext = NULL;
	if (args.Length() > 1)
	{
		v8::String::Utf8Value jsExt(isolate,Handle<v8::String>::Cast(args[1]));
		ext = strdup(*jsExt);
	}
	vector<string> files;
	while ((dirp = readdir(dp)) != NULL) {
		if (strcmp(dirp->d_name, "..")==0 || strcmp(dirp->d_name, ".")==0) continue;
		int lio = ext ? lastIndexOf(dirp->d_name, ext) : -1;
		if (ext && (lio == 0 || lio != (strlen(dirp->d_name)-strlen(ext)))) continue;
		files.push_back(string(dirp->d_name));
    }
	Handle<Array> results = v8::Array::New(isolate, files.size());
	for (int i=0; i<files.size(); i++) {
		results->Set(CONTEXT_ARG i, v8::String::NewFromUtf8(isolate, files[i].c_str())TO_LOCAL_CHECKED);
	}	
	if (ext) free(ext);
    closedir(dp);
	args.GetReturnValue().Set(results);		
}


static void OSCWD(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);
	char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL)
	{
		Throw(isolate, "can't get working directory");
		return;
	}
	args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, cwd)TO_LOCAL_CHECKED);		
}



static void OSFileAndReadable(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);
	if (args.Length() == 0 || !args[0]->IsString())
	{		
		Throw(isolate, "invalid arguments");
		return;
	}	
	v8::String::Utf8Value jsPath(isolate, Handle<v8::String>::Cast(args[0]));	
	args.GetReturnValue().Set(v8::Boolean::New(isolate,isFileReadable(*jsPath)));
}


static void OSDirAndReadable(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);
	if (args.Length() == 0 || !args[0]->IsString())
	{		
		Throw(isolate, "invalid arguments");
		return;
	}	
	v8::String::Utf8Value jsPath(isolate,Handle<v8::String>::Cast(args[0]));	
	args.GetReturnValue().Set(v8::Boolean::New(isolate,isDirReadable(*jsPath)));
}

static void OSLastModifiedTime(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);
	if (args.Length() == 0 || !args[0]->IsString())
	{		
		Throw(isolate, "invalid arguments");
		return;
	}	
	
	v8::String::Utf8Value jsPath(isolate,Handle<v8::String>::Cast(args[0]));	
    struct stat statbuf;
    if (stat(*jsPath, &statbuf) == -1) 
	{
		args.GetReturnValue().Set( v8::Integer::New(isolate, -1));
		return;
    }
	
	args.GetReturnValue().Set( v8::Integer::New(isolate, statbuf.st_mtime));
}



static void OSUnlink(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);
	if (args.Length() == 0 || !args[0]->IsString())
	{		
		Throw(isolate, "invalid arguments");
		return;
	}	
	
	v8::String::Utf8Value jsPath(isolate,Handle<v8::String>::Cast(args[0]));	
	args.GetReturnValue().Set( v8::Integer::New(isolate, unlink(*jsPath)));
}

static void OSRmdir(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);
	if (args.Length() == 0 || !args[0]->IsString())
	{		
		Throw(isolate, "invalid arguments");
		return;
	}	
	
	v8::String::Utf8Value jsPath(isolate,Handle<v8::String>::Cast(args[0]));	
#ifdef _WIN32	
	args.GetReturnValue().Set( v8::Integer::New(isolate, _rmdir(*jsPath)));
#else	
	args.GetReturnValue().Set( v8::Integer::New(isolate, rmdir(*jsPath)));
#endif
}


static void OSFseek(const v8::FunctionCallbackInfo<v8::Value>&  args)
{
    Isolate* isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);

	if (args.Length() < 2)
	{
		Throw(isolate, "invalid arguments");
		return;
	} else if (!args[0]->IsUint32())
	{
		Throw(isolate, "invalid file descriptor");	
		return;
	} else if (!args[1]->IsUint32())
	{
		Throw(isolate, "invalid seek offset");	
		return;
	}
	
	unsigned int fd = args[0]->Uint32Value(isolate->GetCurrentContext()).FromMaybe(0L);
	
	OSContext *ctx = GetOSContext(isolate, args.Holder());

	if (ctx->handles.find(fd) == ctx->handles.end())
	{
		Throw(isolate, "File descriptor not found");
		return;
	}
	
	unsigned int offset = args[1]->Uint32Value(isolate->GetCurrentContext()).FromMaybe(0L);
	fseek(ctx->handles[fd], offset, SEEK_SET); 
}

static void OSFopen(const v8::FunctionCallbackInfo<v8::Value>&  args)
{
    Isolate* isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);

	if (args.Length() != 2 || !args[0]->IsString() || !args[1]->IsString())
	{
	    Throw(isolate, "Unexpected arguments");
		return;
	}
	
	OSContext *ctx = GetOSContext(isolate, args.Holder());
	
	string file(*String::Utf8Value(isolate,args[0]));
	FILE * f = fopen(file.c_str(), (*String::Utf8Value(isolate,args[1])));
	if (!f)
	{
		 Throw(isolate, (string("Error opening file ") + file).c_str());
		 return;
	}
	ctx->num ++;
	ctx->handles[ctx->num] = f; 
	args.GetReturnValue().Set( v8::Integer::New(isolate, ctx->num));
}

static void OSFclose(const v8::FunctionCallbackInfo<v8::Value>&  args)
{
    Isolate* isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);

	if (args.Length() != 1 || !args[0]->IsUint32()) 
	{
		Throw(isolate, "Unexpected arguments");
		return;
	}
	
	OSContext *ctx = GetOSContext(isolate, args.Holder());	
	unsigned int fd = args[0]->Uint32Value(isolate->GetCurrentContext()).FromMaybe(0L);
	
	if (ctx->handles.find(fd) == ctx->handles.end())
	{
		Throw(isolate, "File descriptor not found");
		return;
	}
	fclose(ctx->handles[fd]);
	ctx->handles.erase(ctx->handles.find(fd));
}


static void OSWritebytes(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    Isolate* isolate = args.GetIsolate();
      
    HandleScope outer_scope(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    Context::Scope context_scope(context);


	if (args.Length() < 2 || !args[0]->IsUint32() || !args[1]->IsArray())
	{
		Throw(isolate, "invalid arguments");
		return;
	}
	unsigned int fd = args[0]->Uint32Value(isolate->GetCurrentContext()).FromMaybe(0L);
	OSContext *ctx = GetOSContext(isolate, args.Holder());	

	if (ctx->handles.find(fd) == ctx->handles.end())
	{
		Throw(isolate, "File descriptor not found");
		return;
	}
	
	Handle<Array> jsBytes = Handle<Array>::Cast(args[1]);
	BYTE * buf = new BYTE[jsBytes->Length()];
	int count = jsBytes->Length();
	for (unsigned int i=0; i<count;i++)
	{
		Handle<v8::Value> b =  jsBytes->Get(CONTEXT_ARG i)TO_LOCAL_CHECKED;
		if (!b->IsUint32()) {
			delete[] buf;
			Throw(isolate, "invalid byte array");
			return;
		}
		BYTE byte = b->Uint32Value(isolate->GetCurrentContext()).FromMaybe(0L);
		buf[i] = byte;
	}
	int num = fwrite(buf, 1, count, ctx->handles[fd]);
	delete[] buf;
	args.GetReturnValue().Set(v8::Integer::New(isolate, num));
}	
	

	
static void OSReadbytes(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    Isolate* isolate = args.GetIsolate();
      
    HandleScope outer_scope(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    Context::Scope context_scope(context);

	if (args.Length() < 2 || !args[0]->IsUint32())
	{
		Throw(isolate, "invalid arguments");
		return;
	}

	unsigned int fd = args[0]->Uint32Value(isolate->GetCurrentContext()).FromMaybe(0L);
	OSContext *ctx = GetOSContext(isolate, args.Holder());	

	if (ctx->handles.find(fd) == ctx->handles.end())
	{
		Throw(isolate, "File descriptor not found");
		return;
	}

	if (!args[1]->IsUint32())
	{
		Throw(isolate, "count argument is invalid");
		return;
	}
	
	unsigned int count = args[1]->Uint32Value(isolate->GetCurrentContext()).FromMaybe(0L);
	
	BYTE * buf = new BYTE[count];
	int bytesRead = fread(buf, 1, count, ctx->handles[fd]);
	Handle<Array> jsBytes = v8::Array::New(isolate, bytesRead);
	for (unsigned int i=0; i<bytesRead; i++)
	{
		jsBytes->Set(CONTEXT_ARG i, v8::Integer::New(isolate, buf[i]));
	}
	delete[] buf;
	
	args.GetReturnValue().Set( jsBytes);
	
}

static void OSReadString(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    Isolate* isolate = args.GetIsolate();
      
    HandleScope outer_scope(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    Context::Scope context_scope(context);

	if (args.Length() < 2 || !args[0]->IsUint32())
	{
		Throw(isolate, "invalid arguments");
		return;
	}

	unsigned int fd = args[0]->Uint32Value(isolate->GetCurrentContext()).FromMaybe(0L);
	OSContext *ctx = GetOSContext(isolate, args.Holder());	

	if (ctx->handles.find(fd) == ctx->handles.end())
	{
		Throw(isolate, "File descriptor not found");
		return;
	}

	if (!args[1]->IsUint32())
	{
		Throw(isolate, "count argument is invalid");
		return;
	}
	
	unsigned int count = args[1]->Uint32Value(isolate->GetCurrentContext()).FromMaybe(0L);
	
	char * buf = new char[count+1];
	int bytesRead = fread(buf, 1, count, ctx->handles[fd]);
	buf[bytesRead] = '\0';
	Handle<v8::String> jsVal = v8::String::NewFromUtf8(isolate, buf)TO_LOCAL_CHECKED;
	args.GetReturnValue().Set( jsVal);
	delete[] buf;
}


static void OSWriteString(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);

	if (args.Length() < 2)
	{
		Throw(isolate, "Unexpected arguments");
		return;
	}
	
	unsigned int fd = args[0]->Uint32Value(isolate->GetCurrentContext()).FromMaybe(0L);
	OSContext *ctx = GetOSContext(isolate, args.Holder());	
	
	if (ctx->handles.find(fd) == ctx->handles.end())
	{
		Throw(isolate, "File descriptor not found");
		return;
	}

	if (!args[1]->IsString())
	{
		Throw(isolate, "invalid string argument");
		return;
	}
	
	v8::String::Utf8Value jsString(isolate,Handle<v8::String>::Cast(args[1]));	
	fprintf(ctx->handles[fd], "%s", *jsString);
	
}



static void OSGetHostname(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);
	
	char hostname[1024];
	hostname[1023] = '\0';
	gethostname(hostname, 1023);
	//printf("Hostname: %s\n", hostname);

	Handle<v8::String> jsVal = v8::String::NewFromUtf8(isolate, hostname)TO_LOCAL_CHECKED;
	args.GetReturnValue().Set(jsVal);	  
}


static void OSMkDir(const v8::FunctionCallbackInfo<v8::Value>&  args)
{
    Isolate* isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);

	if (args.Length() != 1 || !args[0]->IsString())
	{
	    Throw(isolate, "Unexpected arguments");
		return;
	}

	v8::String::Utf8Value jsStr(isolate,Handle<v8::String>::Cast(args[0]));
	char * dir = *jsStr;
#ifdef _WIN32
	args.GetReturnValue().Set( v8::Integer::New(isolate, mkdir(dir)));
#else
	args.GetReturnValue().Set( v8::Integer::New(isolate, mkdir(dir,0700)));
#endif	
}



static void OSMkPath(const v8::FunctionCallbackInfo<v8::Value>&  args)
{
    Isolate* isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);

	if (args.Length() != 1 || !args[0]->IsString())
	{
	    Throw(isolate, "Unexpected arguments");
		return;
	}
	
	struct stat sb;
	char *slash;
	int done = 0;

	v8::String::Utf8Value jsStr(isolate,Handle<v8::String>::Cast(args[0]));
	char * path = *jsStr;
	slash = path;
	
	while (!done) {
#ifdef _WIN32
		slash += strspn(slash, "\\");
		slash += strcspn(slash, "\\");
#else
		slash += strspn(slash, "/");
		slash += strcspn(slash, "/");
#endif
		done = (*slash == '\0');
		*slash = '\0';

		if (stat(path, &sb)) {
#ifdef _WIN32
			if (errno != ENOENT || (mkdir(path) &&
#else
			if (errno != ENOENT || (mkdir(path, 0777) &&
#endif 	
			    errno != EEXIST)) {
				return args.GetReturnValue().Set(v8::Integer::New(isolate, -1));	
			}
		} else if (!S_ISDIR(sb.st_mode)) {
			return (-1);
		}

		*slash = '/';
	}

	return args.GetReturnValue().Set(v8::Integer::New(isolate, 0));	

}
	


extern "C" bool LIBRARY_API attach(Isolate* isolate, v8::Local<v8::Context> &context) 
{	
	v8::HandleScope handle_scope(isolate);
    Context::Scope scope(context);
	
	Handle<ObjectTemplate> os = ObjectTemplate::New(isolate);
	
	os->Set(v8::String::NewFromUtf8(isolate, "filesize")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, OSFileSize));
	os->Set(v8::String::NewFromUtf8(isolate, "mkpath")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, OSMkPath));
	os->Set(v8::String::NewFromUtf8(isolate, "mkdir")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, OSMkDir));
	os->Set(v8::String::NewFromUtf8(isolate, "gethostname")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, OSGetHostname));
	os->Set(v8::String::NewFromUtf8(isolate, "cwd")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, OSCWD));
	os->Set(v8::String::NewFromUtf8(isolate, "sleep")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, OSSleep));
	os->Set(v8::String::NewFromUtf8(isolate, "getEnv")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, OSGetEnv));
	os->Set(v8::String::NewFromUtf8(isolate, "setEnv")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, OSSetEnv));
	os->Set(v8::String::NewFromUtf8(isolate, "exec")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, OSExec));
	os->Set(v8::String::NewFromUtf8(isolate, "readFile")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, OSReadFile));
	os->Set(v8::String::NewFromUtf8(isolate, "writeFile")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, OSWriteFile));
	os->Set(v8::String::NewFromUtf8(isolate, "listDir")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, OSListDir));
	os->Set(v8::String::NewFromUtf8(isolate, "isFileAndReadable")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, OSFileAndReadable));
	os->Set(v8::String::NewFromUtf8(isolate, "isDirAndReadable")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, OSDirAndReadable));
	os->Set(v8::String::NewFromUtf8(isolate, "lastModifiedTime")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, OSLastModifiedTime));
	os->Set(v8::String::NewFromUtf8(isolate, "unlink")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, OSUnlink));
	os->Set(v8::String::NewFromUtf8(isolate, "rmdir")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, OSRmdir));
	os->Set(v8::String::NewFromUtf8(isolate, "fseek")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, OSFseek));
	os->Set(v8::String::NewFromUtf8(isolate, "fopen")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, OSFopen));
	os->Set(v8::String::NewFromUtf8(isolate, "fclose")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, OSFclose));
	os->Set(v8::String::NewFromUtf8(isolate, "readBytes")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, OSReadbytes));
	os->Set(v8::String::NewFromUtf8(isolate, "writeString")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, OSWriteString));
	os->Set(v8::String::NewFromUtf8(isolate, "readString")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, OSReadString));
	os->Set(v8::String::NewFromUtf8(isolate, "writeBytes")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, OSWritebytes));
	
	
	os->SetInternalFieldCount(1);  
	
	v8::Local<v8::Object> instance = os->NewInstance(context).ToLocalChecked();	
	OSContext *ctx = new OSContext();	
	ctx->num = 0;
	instance->SetAlignedPointerInInternalField(0, ctx);		
	context->Global()->Set(context,v8::String::NewFromUtf8(isolate,"OS")TO_LOCAL_CHECKED, instance).FromJust();	
	
	return true;
}

extern "C" bool LIBRARY_API init() 
{
#ifdef _WIN32	
	WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif	
 	return true;
}   

  


