/*
Copyright (c) 2020 TINN by Saverio Castellano. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#include "include/v8.h"


#include <sstream>

#ifdef _WIN32
#include <Winsock2.h>
#include <linux.h>
#endif

#include "cluster.h"
#include "hirediscommand.h"

using namespace std;
using namespace v8;
using namespace RedisCluster;

#include "v8adapt.h"

#if defined(_WIN32)
  #define LIBRARY_API __declspec(dllexport)
#else
  #define LIBRARY_API
#endif

#define CONN_RETRY_SECS 10

struct RedisServer
{
	char *host;
	int port;
	
 };
 
typedef struct {
	RedisServer **  servers;
	Cluster<redisContext>::ptr_t cluster;
	int serverNum;
	unsigned int lastConnTs;
} RedisContext;


static Local<Value> Throw(Isolate* isolate, const char* message) {
	return isolate->ThrowException(v8::Exception::Error(String::NewFromUtf8(isolate, message, NewStringType::kNormal).ToLocalChecked()));
}



RedisContext* GetRedisContextFromInternalField(Isolate* isolate, Local<Object> object, bool inited = true) {
  RedisContext* ctx =
  static_cast<RedisContext*>(object->GetAlignedPointerFromInternalField(0));
  if (ctx == NULL) {
    Throw(isolate, "ctx is defunct ");
    return NULL;
  }

  if (inited && !ctx->servers) {
    Throw(isolate, "mod_redis_cluster context not initialized, forgot to call 'connect' ?");
    return NULL;	  
  }
  return ctx;
}


bool Connect(RedisContext *ctx) {	
	//printf("try redis connect\n");
	unsigned int now = (unsigned)time(NULL);
	if (ctx->lastConnTs!=0 && (now-ctx->lastConnTs) < CONN_RETRY_SECS) {
		//printf("can't retry... must wait %d secs\n", (CONN_RETRY_SECS-(now-ctx->lastConnTs)));
		return false;
	}
	
	ctx->lastConnTs = now;
	
	if (ctx->cluster!=NULL) {
	    delete ctx->cluster;
		ctx->cluster = NULL;		
	}
	
	bool anyConnected = false;
	for (int i=0; i<ctx->serverNum; i++){
		RedisServer * svr = ctx->servers[i];
		
		try
		{
			ctx->cluster = HiredisCommand<>::createCluster( svr->host, svr->port );
			return true;
		} catch ( const RedisCluster::ClusterException &e )
		{
			//printf("error create cluster: %p %s:%d\n", ctx->cluster, svr->host , svr->port);
			ctx->cluster = NULL;	
			continue;
		}		
	}  
	return false;
}

static void Hiredis2Connect(const v8::FunctionCallbackInfo<v8::Value>& args) {
    Isolate* isolate = args.GetIsolate();
	
    HandleScope outer_scope(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    Context::Scope context_scope(context);
	
	if (args.Length() == 0 || !args[0]->IsArray())
	{
		  Throw(isolate, "invalid arguments");
		  return;
	}	  
	
	
	RedisContext *ctx = GetRedisContextFromInternalField(isolate, args.Holder());
	if (!ctx) return;
	Handle<Array> svrs = Handle<v8::Array>::Cast(args[0]);
	ctx->servers = new RedisServer *[svrs->Length()];
	ctx->serverNum = svrs->Length();
	ctx->cluster  = NULL;
	ctx->lastConnTs = 0;
	
	  
	for (int i = 0; i< svrs->Length(); i++) {
		if (!svrs->Get(CONTEXT_ARG i)TO_LOCAL_CHECKED ->IsArray()) Throw(isolate, "invalid arguments: each redis server must be an array");
		Handle<Array> svr = Handle<v8::Array>::Cast(svrs->Get(CONTEXT_ARG i)TO_LOCAL_CHECKED);
		if (svr->Length()!=2 || !svr->Get(CONTEXT_ARG 0)TO_LOCAL_CHECKED ->IsString() || !svr->Get(CONTEXT_ARG 1)TO_LOCAL_CHECKED ->IsUint32()) {
			Throw(isolate, "invalid arguments: each redis server must have 2 elements");
		}
		RedisServer * redisServer = new RedisServer();
		redisServer->port =(uint64_t)svr->Get(CONTEXT_ARG 1)TO_LOCAL_CHECKED ->Int32Value(isolate->GetCurrentContext()).FromMaybe(0); 
		v8::String::Utf8Value jsHost(isolate, Handle<v8::String>::Cast(svr->Get(CONTEXT_ARG 0)TO_LOCAL_CHECKED));	
		redisServer->host = strdup(*jsHost);
		//printf("add server %s %d\n", redisServer->host, redisServer->port);
		ctx->servers[i] = redisServer; 
	}
  
	if (!Connect(ctx)) {
		Throw(isolate, "redis connection error");
	}	
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
			//printf("unknown reply=%d\n", reply->type);
		}
		break;
	}
	//printf("return redis reply\n");
	//return  handle_scope.Escape(res);
	return res;
}


static void Hiredis2Command(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope handle_scope(isolate);  
  
	if (args.Length() != 1)
	{		
		Throw(isolate,"invalid arguments");
	} else if (!args[0]->IsString())
	{
		Throw(isolate,"invalid cmd argument");
	}
	
	RedisContext *ctx = GetRedisContextFromInternalField(isolate, args.Holder());
	if (!ctx) return;

	if (ctx->cluster == NULL && !Connect(ctx)) {
		Throw(isolate, "redis connection error");
		return;
	}
	
	v8::String::Utf8Value jsCmd(isolate,Handle<v8::String>::Cast(args[0]));
	sds cmd = sdsnew(*jsCmd);

	sds *tokens;
	int count,j;
	tokens = sdssplitlen(cmd,sdslen(cmd)," ",1,&count);
	
	char * key;
	if (count > 1) {
		key = tokens[1];	
		redisReply * reply = NULL;
		try {
			reply= static_cast<redisReply*>( HiredisCommand<>::Command( ctx->cluster, key, *jsCmd) );
			Handle<Object> res = getReplyObject(isolate, reply);
			if (reply) {
				freeReplyObject(reply);
				reply = NULL;
			}
			args.GetReturnValue().Set (res);		
		} catch (const std::runtime_error &e) {
			//printf("error executing redis cmd\n");
			if (reply) {
				freeReplyObject(reply);
				reply = NULL;
			}			
			if (!Connect(ctx)) {
				Throw(isolate, "redis connection error");
			} else {
				try {
					reply= static_cast<redisReply*>( HiredisCommand<>::Command( ctx->cluster, key, *jsCmd) );
					Handle<Object> res = getReplyObject(isolate, reply);
					if (reply) {
						freeReplyObject(reply);
						reply = NULL;
					}					
					args.GetReturnValue().Set (res);							
				} catch (const std::runtime_error &e) {
					if (reply) {
						freeReplyObject(reply);
						reply = NULL;
					}					
					Throw(isolate, "redis connection error");	
				}
			}
		}
	}		

	//printf("freeing shit\n");
	sdsfreesplitres(tokens,count);
	sdsfree(cmd);
}



static void Hiredis2CommandArgv(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope handle_scope(isolate);  
  
	if (args.Length() < 2)
	{		
		Throw(isolate,"invalid arguments");
		return;
	}

	RedisContext *ctx = GetRedisContextFromInternalField(isolate, args.Holder());
	if (ctx->cluster == NULL && !Connect(ctx)) {
		Throw(isolate, "redis connection error");
		return;
	}	
	
	
	int num = args.Length();	
	v8::String::Utf8Value jsKey(isolate,Handle<v8::String>::Cast(args[1]));
	char *key = (*jsKey);
	const char ** rargs = (const char**)new char *[args.Length()];
	size_t * rargsLen = new size_t[args.Length()];
	
	for (int i=0; i<num; i++)
	{
		if (!args[i]->IsString())
		{
			Throw(isolate,"invalid arguments");
		}
		v8::String::Utf8Value jsArg(isolate,Handle<v8::String>::Cast(args[i]));
		rargs[i]= strdup(*jsArg);
		rargsLen[i] = strlen(*jsArg);
	}
	redisReply * reply;
	try {
		reply= static_cast<redisReply*>( HiredisCommand<>::Command( ctx->cluster, key, num, rargs, rargsLen));		
		Handle<Object> res = getReplyObject(isolate, reply);
		if (reply) {
			freeReplyObject(reply);
			reply = NULL;
		}		
		args.GetReturnValue().Set (res);		
	} catch (const std::runtime_error &e) {
		if (reply) {
			freeReplyObject(reply);
			reply = NULL;
		}			
		if (!Connect(ctx)) {
			Throw(isolate, "redis connection error");
		} else {
			try {
				reply= static_cast<redisReply*>( HiredisCommand<>::Command( ctx->cluster, key, num, rargs, rargsLen));				
				Handle<Object> res = getReplyObject(isolate, reply);
				if (reply) {
					freeReplyObject(reply);
					reply = NULL;
				}				
				args.GetReturnValue().Set (res);							
			} catch (const std::runtime_error &e) {
				if (reply) {
					freeReplyObject(reply);
					reply = NULL;
				}				
				Throw(isolate, "redis connection error");	
			}
		}
	}
	
	for (int i=0; i<num; i++)
	{
		free((char*)rargs[i]);
	}		
	delete[] rargsLen;
	delete[] rargs; 
	
}


extern "C" bool LIBRARY_API attach(Isolate* isolate, v8::Local<v8::Context> &context) 
{
	v8::HandleScope handle_scope(isolate);
    Context::Scope scope(context);
	
	Handle<ObjectTemplate> hiredis = ObjectTemplate::New(isolate);
	hiredis->Set(v8::String::NewFromUtf8(isolate, "connect")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, Hiredis2Connect));
	hiredis->Set(v8::String::NewFromUtf8(isolate,"REPLY_STRING")TO_LOCAL_CHECKED, v8::Integer::New(isolate, REDIS_REPLY_STRING));
	hiredis->Set(v8::String::NewFromUtf8(isolate,"REPLY_ARRAY")TO_LOCAL_CHECKED, v8::Integer::New(isolate, REDIS_REPLY_ARRAY));
	hiredis->Set(v8::String::NewFromUtf8(isolate,"REPLY_INTEGER")TO_LOCAL_CHECKED, v8::Integer::New(isolate, REDIS_REPLY_INTEGER));
	hiredis->Set(v8::String::NewFromUtf8(isolate,"REPLY_NIL")TO_LOCAL_CHECKED, v8::Integer::New(isolate, REDIS_REPLY_NIL));
	hiredis->Set(v8::String::NewFromUtf8(isolate,"REPLY_STATUS")TO_LOCAL_CHECKED, v8::Integer::New(isolate, REDIS_REPLY_STATUS));
	hiredis->Set(v8::String::NewFromUtf8(isolate,"REPLY_ERROR")TO_LOCAL_CHECKED, v8::Integer::New(isolate, REDIS_REPLY_ERROR));
	hiredis->Set(v8::String::NewFromUtf8(isolate, "command")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, Hiredis2Command));
	hiredis->Set(v8::String::NewFromUtf8(isolate, "commandArgv")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, Hiredis2CommandArgv));
	
	hiredis->SetInternalFieldCount(1);  
	
	v8::Local<v8::Object> instance = hiredis->NewInstance(context).ToLocalChecked();	
    RedisContext * ctx = new RedisContext();
	instance->SetAlignedPointerInInternalField(0, ctx);		
	context->Global()->Set(context,v8::String::NewFromUtf8(isolate,"RedisCluster")TO_LOCAL_CHECKED, instance).FromJust();
	return true;
}

extern "C" bool LIBRARY_API init() 
{
	return true;
}

