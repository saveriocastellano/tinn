/*
Copyright (c) 2020 TINN by Saverio Castellano. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#include <string.h>
#include "include/v8.h"
#include "fcgiapp.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "sds.h"
#include <stdio.h> 
#include <stdlib.h>
#include <istream>
#include <fstream>


#include "v8adapt.h"		


using std::ifstream;


static int socketId;
static const unsigned long STDIN_MAX = 1000000;

using namespace v8;

#include <curl/curl.h>
#ifdef _WIN32
#pragma comment(lib,"Ws2_32.lib")
#endif


#if defined(_WIN32)
  #define LIBRARY_API __declspec(dllexport)
#else
  #define LIBRARY_API
#endif



typedef struct {
	FCGX_Request * request;
	CURL * curl;
	bool served;
} HttpContext;
 

int isFileReadable(char * file)
{
	struct stat s;
	return ( stat(file,&s) == 0 && !(s.st_mode & S_IFDIR));
}

size_t writer(char *data, size_t size, size_t nmemb, sds * buf)
{
   if(!buf)
      return 0;
      
   int len = size * nmemb;
   if(len > 0) {
       (*buf) = sdscatlen((*buf), data, len);  
   }
   return len;
}


static Local<Value> Throw(Isolate* isolate, const char* message) {
	return isolate->ThrowException(v8::Exception::Error(String::NewFromUtf8(isolate, message, NewStringType::kNormal).ToLocalChecked()));
}

HttpContext* GetHttpFromInternalField(Isolate* isolate, Local<Object> object) {
  HttpContext* ctx =
      static_cast<HttpContext*>(object->GetAlignedPointerFromInternalField(0));
  if (ctx == NULL) {
    Throw(isolate, "ctx is defunct ");
    return NULL;
  }

  return ctx;
}


static void HttpReset(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope handle_scope(isolate);
  
  	HttpContext *ctx = GetHttpFromInternalField(isolate, args.Holder());
    if (ctx ) {
		if (ctx->curl)
		{
			curl_easy_cleanup(ctx->curl);
		}
		if (ctx->request)
		{
			delete ctx->request;
		}
    }
	args.Holder()->SetAlignedPointerInInternalField(0, NULL);
}

static void HttpGetRequestBody(const v8::FunctionCallbackInfo<v8::Value>& args)
{    
  Isolate* isolate = args.GetIsolate();
  HandleScope handle_scope(isolate);
	
	HttpContext *ctx = GetHttpFromInternalField(isolate, args.Holder());
  if (!ctx || !ctx->request) {
    Throw(isolate,"failed to get request\n");
    return;
  }
  FCGX_Request* request = ctx->request;
	
    char * content_length_str = FCGX_GetParam("CONTENT_LENGTH", request->envp);

    unsigned long content_length = STDIN_MAX;

    if (content_length_str) {
        content_length = strtol(content_length_str, &content_length_str, 10);
        if (*content_length_str) {
			
			Throw(isolate, "error parsing content length");
			return;
        }

        if (content_length > STDIN_MAX) {
            content_length = STDIN_MAX;
        }
    } else {
        content_length = 0;
    }

	
    char * buf = new char[1024];
	int bytesRead = 0;
	sds content = sdsempty();
	do {

		bytesRead = FCGX_GetStr(buf, 1024, request->in);
		content = sdscatlen(content, buf, bytesRead);
		
	} while(bytesRead > 0 && sdslen(content) < content_length);
	
	args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, content)TO_LOCAL_CHECKED);
	sdsfree(content);
	delete [] buf;
  
}

static void HttpGetParam(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope handle_scope(isolate);
	HttpContext *ctx = GetHttpFromInternalField(isolate, args.Holder());
  if (!ctx || !ctx->request) {
    Throw(isolate,"failed to get request\n");
    return;
  }
  FCGX_Request* request = ctx->request;
  if (args.Length() == 0 || !args[0]->IsString())
  {
		Throw (isolate,"invalid arguments");
		return;	
  }  

  v8::String::Utf8Value jsName(isolate,Handle<v8::String>::Cast(args[0]));
  char *val = FCGX_GetParam(*jsName, request->envp);		
  if (!val && args.Length() > 1)
  {
	  args.GetReturnValue().Set(args[1]);
	  return;
	  
  } else if (val)
  {
	  Handle<v8::String> jsVal = v8::String::NewFromUtf8(isolate, val)TO_LOCAL_CHECKED;
	  args.GetReturnValue().Set(jsVal);	  
  }
}



static void HttpAccept(const v8::FunctionCallbackInfo<v8::Value>& args) {
	Isolate* isolate = args.GetIsolate();
	HandleScope handle_scope(isolate);
	HttpContext *ctx = GetHttpFromInternalField(isolate, args.Holder());
	if (!ctx) {
		Throw(isolate,"mod_http: FCGI error initializing request\n");
		return;
	}
	if (!ctx->request) {
		FCGX_Request * request = new FCGX_Request();
		if(FCGX_InitRequest(request, socketId, FCGI_FAIL_ACCEPT_ON_INTR) != 0)
		{
		   Throw(isolate, "mod_http: failed to initialize FCGX_Request\n");
		   delete request;
		   return;
		}
		ctx->request = request;
	}
	
	FCGX_Request* request = ctx->request;
	int rc = FCGX_Accept_r(request);
	ctx->served = false;
	if (rc)
	{
	  
	}
}

static void HttpFinish(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope handle_scope(isolate);
	HttpContext *ctx = GetHttpFromInternalField(isolate, args.Holder());
  if (!ctx || !ctx->request) {
    Throw(isolate,"failed to get request\n");
    return;
  }
  FCGX_Request* request = ctx->request;
  FCGX_Finish_r(request);		
}

static void HttpPrint(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope handle_scope(isolate);
 
	HttpContext *ctx = GetHttpFromInternalField(isolate, args.Holder());
  if (!ctx || !ctx->request) {
    Throw(isolate,"failed to get request\n");
    return;
  }
  if (ctx->served) {
	  return;
  }
  FCGX_Request* request = ctx->request;
  if (args.Length() == 0 || !args[0]->IsString())
  {
		Throw (isolate,"invalid arguments");
		return;	
  }
  v8::String::Utf8Value jsVal(isolate,Handle<v8::String>::Cast(args[0]));
  FCGX_PutS(*jsVal, request->out);
}

static void HttpCloseSocket(const v8::FunctionCallbackInfo<v8::Value>& args) {
	Isolate* isolate = args.GetIsolate();
	HandleScope handle_scope(isolate);
	FCGX_ShutdownPending();
#ifdef _WIN32	
	shutdown(socketId,SD_BOTH);

#else 
	shutdown(socketId,SHUT_RD);
#endif	
}

static void HttpOpenSocket(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope handle_scope(isolate);

  if (args.Length() == 0 || !args[0]->IsString())
  {
		Throw (isolate,"invalid arguments");
		return;	
  }
  
  FCGX_Init();
#ifndef _WIN32  
  umask(0);
#endif  
  v8::String::Utf8Value jsSocket(isolate,Handle<v8::String>::Cast(args[0]));
  socketId = FCGX_OpenSocket(*jsSocket, 2000);
  if(socketId < 0)
  {
		Throw(isolate,"can't open FCGX socket\n");
  }
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


static void HttpServeFile(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);
	
	HttpContext *ctx = GetHttpFromInternalField(isolate, args.Holder());
    if (!ctx || !ctx->request) {
      Throw(isolate,"failed to get request\n");
      return;
    }
	
	if (args.Length() != 2)
	{		
		Throw(isolate,"invalid arguments");
		return;
	}
	if (!args[0]->IsString() || !args[1]->IsString())
	{		
		Throw(isolate,"invalid arguments");
		return;
	}	
	
	v8::String::Utf8Value jsFile(isolate,Handle<v8::String>::Cast(args[0]));
	v8::String::Utf8Value jsMimeType(isolate,Handle<v8::String>::Cast(args[1]));
	
	int size = 0;
	char* chars = ReadChars(isolate, *jsFile, &size);
	if (chars == NULL) return ;
	
	
    FCGX_Request* request = ctx->request;
    
	FCGX_PutS("Status: 200 OK\r\n", request->out);
	char header[256]; 
	sprintf(header, "Content-type: %s\r\n", *jsMimeType);
	FCGX_PutS(header, request->out);
	sprintf(header, "Content-Length: %d\r\n", size);
	FCGX_PutS(header, request->out);
	FCGX_PutS("\r\n", request->out);
	
    FCGX_PutS(chars, request->out);
	delete[] chars;
 	/*int bufsize = 500;	
	char buf[500];
	int read_bytes;
	while (!ifs.eof() && size > 0)
    {
		ifs.read(buf, size < bufsize ? size : bufsize);
		read_bytes = ifs.gcount();
		size -= read_bytes;
		FCGX_PutS(buf, request->out);
		printf("puts: %s\n" , buf);
	}*/
	
	ctx->served = true;
}

char* strtolower(char* s) {
  char* p = s;
  while (*p != '\0') {
    *p = tolower(*p);
    p++;
  }

  return s;
}

static void HttpRequest(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    Isolate* isolate = args.GetIsolate();
			
    HandleScope outer_scope(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    Context::Scope context_scope(context);			
			
	if (args.Length() < 1)
	{		
		Throw(isolate,"invalid arguments");
		return;
	}
	if (!args[0]->IsString() && !args[0]->IsArray())
	{		
		Throw(isolate,"invalid url");
		return;
	}
	HttpContext *ctx = GetHttpFromInternalField(isolate, args.Holder());
	if (!ctx || !ctx->curl) {
		Throw(isolate,"failed to get curl\n");
		return;
	}
	
	CURL * curl = ctx->curl;  

	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);	
	
	v8::String::Utf8Value jsUrl(isolate,Handle<v8::String>::Cast(args[0]));
	char *url = NULL;
	
	if (args[0]->IsString()) {
		url = *jsUrl;
		curl_easy_setopt(curl, CURLOPT_PROXY, NULL);
	} else if (args[0]->IsArray()) {
		Handle<Array> jsUrlParts = Handle<Array>::Cast(args[0]);
		if (jsUrlParts->Length()!=2) {
			Throw(isolate,"url must be a 2-value array or string");
			return;			
		}
		v8::String::Utf8Value jsProxy(isolate,Handle<v8::String>::Cast(jsUrlParts->Get(CONTEXT_ARG 1)TO_LOCAL_CHECKED));
		v8::String::Utf8Value jsUrlFromArray(isolate,Handle<v8::String>::Cast(jsUrlParts->Get(CONTEXT_ARG 0)TO_LOCAL_CHECKED));
#ifdef _WIN32
		url = _strdup(*jsUrlFromArray);
#else 
		url = strdup(*jsUrlFromArray);
#endif	
		curl_easy_setopt(curl, CURLOPT_PROXY, *jsProxy);
	}
	
	v8::String::Utf8Value jsBody(isolate,Handle<v8::String>::Cast(args[3]));
	v8::String::Utf8Value jsMethod(isolate,Handle<v8::String>::Cast(args[1]));

	sds buf = sdsempty();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_HTTPGET, true);
	
	if (args[0]->IsArray()) free(url);	

	if (args.Length() > 1 && strcmp(*jsMethod, "")!=0)
	{
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, *jsMethod);
	} else
	{
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, NULL);
	}
		
	if (args.Length() > 3 && strcmp(strtolower(*jsMethod),"post")==0)
	{
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, *jsBody);
	}
	
	long timeout = 60L;
	if (args.Length() > 4)
	{
		timeout = args[4]->Uint32Value(isolate->GetCurrentContext()).FromMaybe(60L);
	}
	
	if (args.Length() > 2)
	{
		if (!args[2]->IsArray())
		{
			sdsfree(buf);
			Throw(isolate, "invalid value for headers");	
			return;
		}
	
		Handle<Array> jsHeaders = Handle<Array>::Cast(args[2]);
 
		//headers..
		struct curl_slist *headers = NULL;
		for (unsigned int i=0; i<jsHeaders->Length();i++)
		{
			v8::String::Utf8Value header(isolate,Handle<v8::String>::Cast(jsHeaders->Get(CONTEXT_ARG i)TO_LOCAL_CHECKED));
			headers = curl_slist_append(headers, *header);		
		}
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); 		
	} else 
	{
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, NULL); 		
	}
	
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, timeout);

    CURLcode result = curl_easy_perform(curl);
	Local<Object> res = Object::New(isolate);	
	res->Set(CONTEXT_ARG v8::String::NewFromUtf8(isolate,"result")TO_LOCAL_CHECKED, v8::Integer::New(isolate,(int)result));
	res->Set(CONTEXT_ARG v8::String::NewFromUtf8(isolate,"response")TO_LOCAL_CHECKED, v8::String::NewFromUtf8(isolate, buf)TO_LOCAL_CHECKED);
	
	args.GetReturnValue().Set(res);	
	sdsfree(buf);
}


static void HttpInit(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope handle_scope(isolate);
  
  HttpContext * ctx = new HttpContext();
  FCGX_Request * request = new FCGX_Request();
  if(FCGX_InitRequest(request, socketId, FCGI_FAIL_ACCEPT_ON_INTR) != 0)
  {
	   delete ctx;
	   Throw(isolate, "failed to init FCGX request");
       return;
  }
  
  CURL *curl = curl_easy_init();
  if (!curl)
  {
	   delete ctx;
	   Throw(isolate, "failed to init CURL");
       return;
	  
  }
    curl_easy_setopt(curl, CURLOPT_HEADER, 1);
    curl_easy_setopt(curl, CURLOPT_HTTP_TRANSFER_DECODING, 0);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Hypergaming");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writer);
	
	ctx->curl = curl;
	ctx->request = request;
	args.Holder()->SetAlignedPointerInInternalField(0, ctx);
}


extern "C" bool LIBRARY_API attach(Isolate* isolate, v8::Local<v8::Context> &context) 
{	
	v8::HandleScope handle_scope(isolate);
    Context::Scope scope(context);
	
	Handle<ObjectTemplate> http = ObjectTemplate::New(isolate);
	
	http->Set(v8::String::NewFromUtf8(isolate, "reset")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, HttpReset));
	http->Set(v8::String::NewFromUtf8(isolate, "init")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, HttpInit));
	http->Set(v8::String::NewFromUtf8(isolate, "accept")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, HttpAccept));
	http->Set(v8::String::NewFromUtf8(isolate, "getParam")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, HttpGetParam));
	http->Set(v8::String::NewFromUtf8(isolate, "getRequestBody")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, HttpGetRequestBody));
	http->Set(v8::String::NewFromUtf8(isolate, "finish")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, HttpFinish));
	http->Set(v8::String::NewFromUtf8(isolate, "print")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, HttpPrint));
	http->Set(v8::String::NewFromUtf8(isolate, "openSocket")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, HttpOpenSocket));
	http->Set(v8::String::NewFromUtf8(isolate, "closeSocket")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, HttpCloseSocket));
	http->Set(v8::String::NewFromUtf8(isolate, "request")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, HttpRequest));
	http->Set(v8::String::NewFromUtf8(isolate, "serveFile")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, HttpServeFile));
	
	http->SetInternalFieldCount(1);  
		
	HttpContext * ctx = new HttpContext();
	ctx->request = NULL;
	
	CURL *curl = curl_easy_init();
	if (!curl)
	{
	   printf("mod_http attach error: failed to initialize curl\n");
	   delete ctx;
	   return false;
	  
	}
	curl_easy_setopt(curl, CURLOPT_HEADER, 1);
	curl_easy_setopt(curl, CURLOPT_HTTP_TRANSFER_DECODING, 0);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "TINN");
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writer);

	ctx->curl = curl;
	
	v8::Local<v8::Object> instance = http->NewInstance(context).ToLocalChecked();
	instance->SetAlignedPointerInInternalField(0, ctx);		
	context->Global()->Set(context,v8::String::NewFromUtf8(isolate,"Http")TO_LOCAL_CHECKED, instance).FromJust();
	
	return true;
}

extern "C" bool LIBRARY_API init() 
{
 	return true;
}




