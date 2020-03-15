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
#include "hiredis.h"
#include <sstream>

#ifdef _WIN32
#include <Winsock2.h>
#include <linux.h>
#endif

#include "v8adapt.h"	
	
#if defined(_WIN32)
  #define LIBRARY_API __declspec(dllexport)
#else
  #define LIBRARY_API
#endif

typedef redisContext redisContext_t;

using namespace std;
using namespace v8;

#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT 6379


typedef struct {
	char *host;
	int port;
} RedistConnectArgs;

typedef struct {
	RedistConnectArgs * args;
	redisContext * conn;	
	
} RedisContext;

static Local<Value> Throw(Isolate* isolate, const char* message) {
	return isolate->ThrowException(v8::Exception::Error(String::NewFromUtf8(isolate, message, NewStringType::kNormal).ToLocalChecked()));
}

RedisContext* GetRedisContextFromInternalField(Isolate* isolate, Local<Object> object) {
  RedisContext* ctx =
  static_cast<RedisContext*>(object->GetAlignedPointerFromInternalField(0));
  if (ctx == NULL) {
    Throw(isolate, "ctx is defunct ");
    return NULL;
  }
  return ctx;
}


bool connect(Isolate *isolate, const v8::FunctionCallbackInfo<v8::Value>& args)
{
   RedisContext *ctx = GetRedisContextFromInternalField(isolate, args.Holder());
	   
   if (ctx->conn)
   {
	   redisFree(ctx->conn);
   }
   
	struct timeval timeout = { 1, 500000 }; // 1.5 seconds
	
	ctx->conn = redisConnectWithTimeout(ctx->args->host, ctx->args->port, timeout);	
	
	if (ctx->conn != NULL && ctx->conn->err) {
		
		redisFree(ctx->conn);
		ctx->conn = NULL;
		
	   return false;
	} else if (!ctx->conn)
	{		
	   return false;	
	}
		
	return true;
}


Local<Object> getReplyObject(Isolate * isolate, redisReply *reply)
{  
    //HandleScope outer_scope(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    Context::Scope context_scope(context);	
	
	Local<Object> res = Object::New(isolate);	
	res->Set(CONTEXT_ARG v8::String::NewFromUtf8(isolate,"type")TO_LOCAL_CHECKED, v8::Integer::New(isolate,reply->type));

	switch (reply->type)
	{
		case REDIS_REPLY_ARRAY:
		{
			//printf("redis reply array\n");
			Handle<Array> elements = v8::Array::New(isolate,reply->elements);
			for (unsigned int i= 0; i<reply->elements; i++)
			{
				elements->Set(CONTEXT_ARG i, getReplyObject(isolate, reply->element[i]));
			}
			res->Set(CONTEXT_ARG v8::String::NewFromUtf8(isolate,"elements")TO_LOCAL_CHECKED, elements);
		}
			//res->Set(String::New("elements"),  v8::Integer::New(reply->elements));
		break;
		case REDIS_REPLY_STATUS:
		case REDIS_REPLY_STRING:
		{
			//printf("redis reply string=%s\n", reply->str);
			res->Set(CONTEXT_ARG v8::String::NewFromUtf8(isolate,"string")TO_LOCAL_CHECKED, v8::String::NewFromUtf8(isolate,reply->str)TO_LOCAL_CHECKED);	
		}
		break;
		case REDIS_REPLY_INTEGER:
		{
			//printf("redis reply integer=%d\n", reply->integer);
			res->Set(CONTEXT_ARG v8::String::NewFromUtf8(isolate,"integer")TO_LOCAL_CHECKED, v8::Integer::New(isolate,reply->integer));
		}
		break;
		
		default:
		{
			//printf("unknown reply=%d", reply->type);
		}
		break;
	}
	//printf("return redis reply\n");
	//return  handle_scope.Escape(res);
	return res;
}



static void HiredisConnect(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope handle_scope(isolate);  
  
  if ((args.Length() > 0 && !args[0]->IsString()) || (args.Length() > 1 && !args[1]->IsUint32()))
  {
	  Throw(isolate, "invalid arguments");
	  return;
  }
  
  RedisContext *ctx = GetRedisContextFromInternalField(isolate, args.Holder());
  
  if (args.Length()>1)
  {
	  ctx->args->port = args[1]->Uint32Value(isolate->GetCurrentContext()).FromMaybe(60L);
  }
  
  if (args.Length()>0)
  {
	  v8::String::Utf8Value jsHost(isolate,Handle<v8::String>::Cast(args[0]));	  
	  free(ctx->args->host);
	  ctx->args->host = strdup(*jsHost);
  }
  args.GetReturnValue().Set(v8::Boolean::New(isolate, connect(isolate, args)));
 }

 
 
 
static void HiredisClose(const v8::FunctionCallbackInfo<v8::Value>& args) {
    Isolate* isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);  
 
    RedisContext *ctx = GetRedisContextFromInternalField(isolate, args.Holder());
	//printf("closing??\n");
    if (!ctx->conn) return;
	//printf("calling redis free conn=%p fd=%d\n", ctx->conn, ctx->conn->fd);
	redisFree(ctx->conn);
	ctx->conn = NULL;
	
}
 
 

static void HiredisAppendCommand(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope handle_scope(isolate);  
  
	if (args.Length() != 1)
	{		
		Throw(isolate,"invalid arguments");
	} else if (!args[0]->IsString())
	{
		Throw(isolate,"invalid command argument");
	}
	v8::String::Utf8Value jsCmd(isolate,Handle<v8::String>::Cast(args[0]));
	char * cmd = *jsCmd;

    RedisContext *ctx = GetRedisContextFromInternalField(isolate, args.Holder());
	
	if (!ctx->conn || ctx->conn->err)
	{		
		Throw(isolate, "error connecting to redis");
		return;
	}
	
	redisAppendCommand(ctx->conn, cmd);  
}

static void HiredisGetReply(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope handle_scope(isolate);  
  

    RedisContext *ctx = GetRedisContextFromInternalField(isolate, args.Holder());
	
	if (!ctx->conn || ctx->conn->err)
	{
		
		if (!connect(isolate, args)) {
			Throw(isolate, "error connecting to redis");
			return;
		} 
	}
	
	redisReply *reply;
#ifdef _WIN32	
	redisGetReply(ctx->conn,(void**)&reply);
#else	
	redisGetReply(ctx->conn,&reply);
#endif	
	if (reply == NULL)
	{
		Throw(isolate, "error connecting to redis");
		return;
	}
		
	Handle<Object> res = getReplyObject(isolate, reply);
	freeReplyObject(reply);
	args.GetReturnValue().Set(res);	

}


static void HiredisCommand(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope handle_scope(isolate);  
  
 
	if (args.Length() != 1)
	{		
		Throw(isolate,"invalid arguments");
	} else if (!args[0]->IsString())
	{
		Throw(isolate,"invalid command argument");
	}
	v8::String::Utf8Value jsCmd(isolate,Handle<v8::String>::Cast(args[0]));
	char * cmd = *jsCmd;

    RedisContext *ctx = GetRedisContextFromInternalField(isolate, args.Holder());
	
	if (!ctx->conn || ctx->conn->err)
	{
		
		if (!connect(isolate, args)) {
			Throw(isolate, "error connecting to redis");
			return;
		} 
	}
	
	redisReply *reply = (redisReply *)redisCommand(ctx->conn, cmd);
	
	//we might have got a NULL reply because redis got disconnected, in this case re-try connecting
	if (reply == NULL )
	{
		if (!connect(isolate, args))
		{
			Throw(isolate, "error connecting to redis");
			return;
		}
		reply = (redisReply *)redisCommand(ctx->conn, cmd);
	}
	
	if (reply == NULL)
	{
		Throw(isolate,"error executing redis command");
		return;
	}

	Handle<Object> res = getReplyObject(isolate, reply);
	freeReplyObject(reply);
	args.GetReturnValue().Set (res);	
  
}

static void HiredisCommandArgv(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope handle_scope(isolate);  

	if (args.Length() == 0)
	{		
		Throw(isolate,"invalid arguments");
	} 
	
	int num = args.Length();
	const char * redisArgs[num];
	stringstream ss;
	for (int i= 0; i<num; i++)
	{
		if (!args[i]->IsString())
		{
			Throw(isolate,"invalid arguments");
		}
		v8::String::Utf8Value jsArg(isolate,Handle<v8::String>::Cast(args[i]));
		redisArgs[i] = strdup(*jsArg);
		//printf("redis arg %i: %s\n", i, redisArgs[i]);
		ss << redisArgs[i] << " ";
	}
	
	RedisContext * ctx = GetRedisContextFromInternalField(isolate, args.Holder());
	if (!ctx->conn || ctx->conn->err)
	{
		if (!connect(isolate, args)) {
			Throw(isolate, "error connecting to redis");
			for (int i= 0; i<num; i++) free((char*)redisArgs[i]);
			return;
		} 
	}

	redisReply *reply = (redisReply *)redisCommandArgv(ctx->conn, num, redisArgs, NULL);
	//we might have got a NULL reply because redis got disconnected, in this case re-try connecting
	if (reply == NULL )
	{
		if (!connect(isolate, args))
		{  
			Throw(isolate, "error connecting to redis");
			for (int i= 0; i<num; i++) free((char*)redisArgs[i]);
			return;
		}
		reply = (redisReply *)redisCommandArgv(ctx->conn, num, redisArgs, NULL);
	}
	
	if (reply == NULL)
	{
		Throw(isolate, "redis error when executing command");
		for (int i= 0; i<num; i++) free((char*)redisArgs[i]);
		return;
	}
	Handle<Object> res = getReplyObject(isolate, reply);
	freeReplyObject(reply);
	for (int i= 0; i<num; i++) free((char*)redisArgs[i]);
	args.GetReturnValue().Set(res);	
  
}



extern "C" bool LIBRARY_API attach(Isolate* isolate, v8::Local<v8::Context> &context) 
{
	Handle<ObjectTemplate> hiredis = ObjectTemplate::New(isolate);
	
	hiredis->Set(v8::String::NewFromUtf8(isolate, "close")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, HiredisClose));
	hiredis->Set(v8::String::NewFromUtf8(isolate, "connect")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, HiredisConnect));
	hiredis->Set(v8::String::NewFromUtf8(isolate, "command")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, HiredisCommand));
	hiredis->Set(v8::String::NewFromUtf8(isolate, "commandArgv")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, HiredisCommandArgv));
	hiredis->Set(v8::String::NewFromUtf8(isolate, "appendCommand")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, HiredisAppendCommand));
	hiredis->Set(v8::String::NewFromUtf8(isolate, "getReply")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, HiredisGetReply));
		
	hiredis->Set(v8::String::NewFromUtf8(isolate,"REPLY_STRING")TO_LOCAL_CHECKED, v8::Integer::New(isolate, REDIS_REPLY_STRING));
	hiredis->Set(v8::String::NewFromUtf8(isolate,"REPLY_ARRAY")TO_LOCAL_CHECKED, v8::Integer::New(isolate, REDIS_REPLY_ARRAY));
	hiredis->Set(v8::String::NewFromUtf8(isolate,"REPLY_INTEGER")TO_LOCAL_CHECKED, v8::Integer::New(isolate, REDIS_REPLY_INTEGER));
	hiredis->Set(v8::String::NewFromUtf8(isolate,"REPLY_NIL")TO_LOCAL_CHECKED, v8::Integer::New(isolate, REDIS_REPLY_NIL));
	hiredis->Set(v8::String::NewFromUtf8(isolate,"REPLY_STATUS")TO_LOCAL_CHECKED, v8::Integer::New(isolate, REDIS_REPLY_STATUS));
	hiredis->Set(v8::String::NewFromUtf8(isolate,"REPLY_ERROR")TO_LOCAL_CHECKED, v8::Integer::New(isolate, REDIS_REPLY_ERROR));
	
	hiredis->SetInternalFieldCount(1);  
	
	v8::Local<v8::Object> instance = hiredis->NewInstance(context).ToLocalChecked();	
	RedisContext *ctx = new RedisContext();
	ctx->args = new RedistConnectArgs();
	ctx->args->port = DEFAULT_PORT;
	ctx->args->host = strdup(DEFAULT_HOST);	
	instance->SetAlignedPointerInInternalField(0, ctx);		
	context->Global()->Set(context,v8::String::NewFromUtf8(isolate,"Redis")TO_LOCAL_CHECKED, instance).FromJust();	
	return true;
}

extern "C" bool LIBRARY_API init() 
{
	return true;
}




