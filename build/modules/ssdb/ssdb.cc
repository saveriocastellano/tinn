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
#include <algorithm>  
#include <math.h>


#ifdef _WIN32
#include <Winsock2.h>
#include <linux.h>
#endif 

#include "SSDB_client.h"

#include "v8adapt.h"	
	
#if defined(_WIN32)
  #define LIBRARY_API __declspec(dllexport)
#else
  #define LIBRARY_API
#endif

using namespace std;
using namespace v8;

#define INSTANCE_CONN_RETRY_SECS 300

// crc16 for computing redis cluster slot
static const uint16_t crc16Table[256] =
{
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
    0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
    0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
    0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
    0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
    0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
    0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
    0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
    0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
    0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
    0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
    0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
    0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
    0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
    0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
    0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
    0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
    0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};

uint16_t CRC16(const char *pszData, int nLen)
{
    int nCounter;
    uint16_t nCrc = 0;
    for (nCounter = 0; nCounter < nLen; nCounter++)
        nCrc = (nCrc << 8) ^ crc16Table[((nCrc >> 8) ^ * pszData++) & 0x00FF];
    return nCrc;
}


struct Instance {
    std::string strHost;
    int nPort;
	ssdb::Client *conn;	
	unsigned int lastConnTs;
};

struct SlotRegion
{
	Instance ** instances;
	int instanceNum;
	Instance * currentInstance;
 };

typedef struct {
	SlotRegion **  regions;
	int regionNum;
} SSDBContext;

static Local<Value> Throw(Isolate* isolate, const char* message) {
  //printf("SSDB throw exception: %s\n" , message);
  return isolate->ThrowException(v8::Exception::Error(String::NewFromUtf8(isolate, message, NewStringType::kNormal).ToLocalChecked()));
}

bool ConnectInstance(Instance *inst)
{
    if (inst->conn)
    {
	   printf("connect instance deleting conn\n");
	   delete inst->conn;
	   inst->conn = NULL;
    }
    inst->conn = ssdb::Client::connect(inst->strHost, inst->nPort);
	inst->lastConnTs = (unsigned)time(NULL);
	if (inst->conn == NULL) {
	   printf("ssdb instance connect error, host=%s port=%d\n", inst->strHost.c_str(), inst->nPort);
	   return false;
	}
	//printf("region connected: host=%s port=%d\n", region->strHost.c_str(), region->nPort);
	return true;
}


SlotRegion * GetSSDBRegion(Local<Object> object, char *key) {
    SSDBContext* ctx = static_cast<SSDBContext*>(object->GetAlignedPointerFromInternalField(0));
		
    int len = strlen(key);
	uint16_t crc16 = CRC16(key, len);
    int nodeId = crc16 % ctx->regionNum;
	SlotRegion * region = ctx->regions[nodeId]; 
	return region;
}

Instance * GetSSDBInstanceFromRegion(SlotRegion * region) {
	unsigned int now = (unsigned)time(NULL);

	for (int i=0; i< region->instanceNum; i++){
		//printf("checking instance i=%d\n", i);
		//printf("is connected? --> %d\n", (region->instances[i]->conn != NULL));
		
		if (region->instances[i]->conn == NULL) {
			//printf("checking if we can reconnect\n");
			if ((now-region->instances[i]->lastConnTs) > INSTANCE_CONN_RETRY_SECS) {
				//printf("calling connect instance\n");
				if (ConnectInstance(region->instances[i])) {
					//printf("reconnected instance=%d\n", i);
					//disconnect others...
					if (i <(region->instanceNum-1)) {
						for (int j=i+1; j<region->instanceNum; j++){
							if (region->instances[j]->conn != NULL) {
								//printf("disconneting instance=%d %s:%d\n", j, region->instances[j]->strHost.c_str(), region->instances[j]->nPort );
								delete region->instances[j]->conn;
								region->instances[j]->conn = NULL;
								region->instances[j]->lastConnTs = 0;
								//printf("freed instance\n");
							}
						}
					}
					//printf("returning (RECONNECTED) instance=%d %s:%d\n", i, region->instances[i]->strHost.c_str(), region->instances[i]->nPort );
					return region->instances[i];
				} else {
					//printf("instance failed to reconnect\n");
				}				
			} else {
				//printf("waiting %d secs before reconnect attempt\n", (INSTANCE_CONN_RETRY_SECS - (now-region->instances[i]->lastConnTs)));
			}			
		} else {	
			//printf("returning instance=%d %s:%d\n", i, region->instances[i]->strHost.c_str(), region->instances[i]->nPort );
			return region->instances[i];			
		}
	}
	//printf("return null!\n");
	return NULL;		
}

SSDBContext* GetSSDBContextFromInternalField(Local<Object> object) {
  SSDBContext* ctx = static_cast<SSDBContext*>(object->GetAlignedPointerFromInternalField(0));
  return ctx;
}



Instance * GetSSDBInstance(Local<Object> object, char *key) {
  
    SSDBContext* ctx = static_cast<SSDBContext*>(object->GetAlignedPointerFromInternalField(0));
    int len = strlen(key);
	uint16_t crc16 = CRC16(key, len);
    int nodeId = crc16 % ctx->regionNum;
  
	//printf("GetSSDBClient hash key to region: %d \n", nodeId);
	return GetSSDBInstanceFromRegion(ctx->regions[nodeId]);
}




static void SSDBGetNodeId(const v8::FunctionCallbackInfo<v8::Value>& args) {
    Isolate* isolate = args.GetIsolate();
    HandleScope outer_scope(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    Context::Scope context_scope(context);
    if (args.Length() != 1 || !args[0]->IsString())
    {
	 Throw(isolate, "invalid arguments");
	  return;
    }	
	
	SSDBContext * ctx = GetSSDBContextFromInternalField(args.Holder());
	
	int num =  ctx->regionNum;
    v8::String::Utf8Value jsArg1(isolate,Handle<v8::String>::Cast(args[0]));	  
	
	char * key = *jsArg1;
    int len = strlen(key);
	uint16_t crc16 = CRC16(key, len);
    int nodeId = crc16 % num;

	args.GetReturnValue().Set (v8::Integer::New(isolate, nodeId));
}

static void SSDBConnect(const v8::FunctionCallbackInfo<v8::Value>& args) {
    Isolate* isolate = args.GetIsolate();
  
    HandleScope outer_scope(isolate);
   Local<Context> context = isolate->GetCurrentContext();
    Context::Scope context_scope(context);
	
	if (args.Length() == 0 || !args[0]->IsArray())
	{
		  Throw(isolate, "invalid arguments");
		  return;
	}	
	   
	srand(time(NULL));
	
	SSDBContext * ctx = GetSSDBContextFromInternalField(args.Holder());
	
	   
	Handle<Array> regions = Handle<v8::Array>::Cast(args[0]);
	ctx->regions = new SlotRegion *[regions->Length()];
	ctx->regionNum = regions->Length();
	  
	for (int i = 0; i< regions->Length(); i++) {
		if (!regions->Get(CONTEXT_ARG i)TO_LOCAL_CHECKED ->IsArray()) Throw(isolate, "invalid arguments: each region must be an array");
		Handle<Array> servers = Handle<v8::Array>::Cast(regions->Get(CONTEXT_ARG i)TO_LOCAL_CHECKED);
		
		SlotRegion * region = new SlotRegion();
		region->instanceNum = servers->Length();
		region->instances = new Instance *[servers->Length()];
		
		for (int j = 0; j< servers->Length(); j++) {
			Handle<Array> svr = Handle<Array>::Cast(servers->Get(CONTEXT_ARG j)TO_LOCAL_CHECKED);
			if (svr->Length()!=2 || !svr->Get(CONTEXT_ARG 0)TO_LOCAL_CHECKED ->IsString() || !svr->Get(CONTEXT_ARG 1)TO_LOCAL_CHECKED ->IsUint32()) {
				Throw(isolate, "invalid arguments: each server must have 2 elements");
			}
			Instance * inst = new Instance();
			inst->lastConnTs = 0;
			inst->nPort =(uint64_t)svr->Get(CONTEXT_ARG 1)TO_LOCAL_CHECKED ->Int32Value(isolate->GetCurrentContext()).FromMaybe(0); 
			v8::String::Utf8Value jsHost(isolate, Handle<v8::String>::Cast(svr->Get(CONTEXT_ARG 0)TO_LOCAL_CHECKED));	
			inst->strHost = (*jsHost);
			inst->lastConnTs = 0;
			region->instances[j] = inst; 
			//printf("adding instances=%d %s:%d to region %d\n", j, inst->strHost.c_str(), inst->nPort , i);
			if (j == 0) ConnectInstance(inst);
		}
		ctx->regions[i] = region;
		
	}
	
}	


	
static void SSDBGetNodeForId(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = args.GetIsolate();
    
    HandleScope outer_scope(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    Context::Scope context_scope(context);
 
  if (args.Length() != 1 || !args[0]->IsInt32())
  {
	 Throw(isolate, "invalid arguments");
	  return;
  }	
  
  int64_t nodeId =  (int64_t)args[0]->Int32Value(isolate->GetCurrentContext()).FromMaybe(0);
///
    Handle<External> field = Handle<External>::Cast(args.Holder()->GetInternalField(0));
    void* ptr = field->Value();
    SSDBContext* ctx =  static_cast<SSDBContext*>(ptr);
///
  Instance * inst = NULL;
  inst = GetSSDBInstanceFromRegion(ctx->regions[nodeId]);	
  //std::string strHost;
  //int nPort;
  Local<Object> res = Object::New(isolate);
  res->Set(CONTEXT_ARG v8::String::NewFromUtf8(isolate,"port")TO_LOCAL_CHECKED, v8::Integer::New(isolate, inst->nPort));
  res->Set(CONTEXT_ARG v8::String::NewFromUtf8(isolate,"host")TO_LOCAL_CHECKED, v8::String::NewFromUtf8(isolate,inst->strHost.c_str())TO_LOCAL_CHECKED);
  args.GetReturnValue().Set (res);

}	

	
static void SSDBNodeRequest(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = args.GetIsolate();
    
    HandleScope outer_scope(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    Context::Scope context_scope(context);
 
  if (args.Length() != 2 || !args[1]->IsArray() || !args[0]->IsInt32())
  {
	 Throw(isolate, "invalid arguments");
	  return;
  }	
  
  Handle<Array> rargs = Handle<v8::Array>::Cast(args[1]);
  if (rargs->Length() < 2)   Throw(isolate, "invalid arguments in array");
  std::vector<std::string> reqArgs;
  for (unsigned int i=0; i<rargs->Length();i++)
  {
	  if (!rargs->Get(CONTEXT_ARG i)TO_LOCAL_CHECKED ->IsString()) Throw(isolate, "invalid arguments");
		v8::String::Utf8Value jsArg(isolate,Handle<v8::String>::Cast(rargs->Get(CONTEXT_ARG i)TO_LOCAL_CHECKED));
		reqArgs.push_back(std::string(*jsArg));
  }  
	
  int64_t nodeId =  (int64_t)args[0]->Int32Value(isolate->GetCurrentContext()).FromMaybe(0);
	
  Instance * inst = NULL;
  Local<Object> res = Object::New(isolate);	
  const std::vector<std::string> *resp;

///
    Handle<External> field = Handle<External>::Cast(args.Holder()->GetInternalField(0));
    void* ptr = field->Value();
    SSDBContext* ctx =  static_cast<SSDBContext*>(ptr);
///

  do {
	   inst = GetSSDBInstanceFromRegion(ctx->regions[nodeId]);
	   if (inst != NULL) {
		   resp = inst->conn->request(reqArgs);
		   ssdb::Status s = ssdb::Status(resp);
		   if (s.server_error()) {
			   delete inst->conn;		   
			   inst->conn = NULL;
			   continue;
		   } else if (!s.ok()) {			
			   res->Set(CONTEXT_ARG v8::String::NewFromUtf8(isolate,"status")TO_LOCAL_CHECKED, v8::Integer::New(isolate,0));
			   args.GetReturnValue().Set (res);	
			   return; 		
		   } else {
			   res->Set(CONTEXT_ARG v8::String::NewFromUtf8(isolate,"status")TO_LOCAL_CHECKED, v8::Integer::New(isolate,1));		   
			   Handle<Array> eRes = v8::Array::New(isolate,resp->size());
			   for (unsigned int i= 0; i<resp->size(); i++)
			   {
			 	  eRes->Set(CONTEXT_ARG i, v8::String::NewFromUtf8(isolate,(*resp)[i].c_str())TO_LOCAL_CHECKED);
			   }
			   res->Set(CONTEXT_ARG v8::String::NewFromUtf8(isolate,"response")TO_LOCAL_CHECKED, eRes);   
			   args.GetReturnValue().Set (res);    
			   return;	
		   }   
	   }
  } while(inst!=NULL); 
  Throw(isolate, "ssdb connect error");  
}	
	
	
static void SSDBRequest(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = args.GetIsolate();
    
    HandleScope outer_scope(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    Context::Scope context_scope(context);
  if (args.Length() != 1 || !args[0]->IsArray())
  {
	 Throw(isolate, "invalid arguments");
	  return;
  }	
  
  Handle<Array> rargs = Handle<v8::Array>::Cast(args[0]);
  if (rargs->Length() < 2)   Throw(isolate, "invalid arguments");
  std::vector<std::string> reqArgs;
  for (unsigned int i=0; i<rargs->Length();i++)
  {
	  if (!rargs->Get(CONTEXT_ARG i)TO_LOCAL_CHECKED ->IsString()) Throw(isolate, "invalid arguments");
		v8::String::Utf8Value jsArg(isolate,Handle<v8::String>::Cast(rargs->Get(CONTEXT_ARG i)TO_LOCAL_CHECKED));
		reqArgs.push_back(std::string(*jsArg));
  }  
 
  v8::String::Utf8Value jsCmd(isolate,Handle<v8::String>::Cast(rargs->Get(CONTEXT_ARG 1)TO_LOCAL_CHECKED)); 
  Instance * inst = NULL;
  Local<Object> res = Object::New(isolate);	
  const std::vector<std::string> *resp;
 
  do {
	   inst = GetSSDBInstance(args.Holder(), *jsCmd);
 	   if (inst != NULL) {
		   resp = inst->conn->request(reqArgs);
		   ssdb::Status s = ssdb::Status(resp);
		   if (s.server_error()) {
			   delete inst->conn;
			   inst->conn = NULL;
			   continue;
		   } else if (!s.ok()) {	
			   res->Set(CONTEXT_ARG v8::String::NewFromUtf8(isolate,"status")TO_LOCAL_CHECKED, v8::Integer::New(isolate,0));
			   args.GetReturnValue().Set (res);	
			   return; 		
		   } else {
			   res->Set(CONTEXT_ARG v8::String::NewFromUtf8(isolate,"status")TO_LOCAL_CHECKED, v8::Integer::New(isolate,1));		   			  
			   Handle<Array> eRes = v8::Array::New(isolate,resp->size());
			   for (unsigned int i= 0; i<resp->size(); i++)
			   {
			 	  eRes->Set(CONTEXT_ARG i, v8::String::NewFromUtf8(isolate,(*resp)[i].c_str())TO_LOCAL_CHECKED);
			   }
			   res->Set(CONTEXT_ARG v8::String::NewFromUtf8(isolate,"response")TO_LOCAL_CHECKED, eRes);   
			   args.GetReturnValue().Set (res);    
			   return;	
		   }   
	   }
  } while(inst!=NULL); 
  Throw(isolate, "ssdb connect error");  
}	
		   
	



static void SSDBPipelinedCommands(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = args.GetIsolate();
     HandleScope outer_scope(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    Context::Scope context_scope(context);
  if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsArray())
  {
	 Throw(isolate, "invalid arguments");
	  return;
  }	
  v8::String::Utf8Value jsArg1(isolate, Handle<v8::String>::Cast(args[0]));	  
  char * arg1 = *jsArg1;

  Handle<Array> cmds = Handle<v8::Array>::Cast(args[1]);
  int num = cmds->Length();
  //std::vector<std::vector <std::string>> commands;
  std::map<SlotRegion *, std::vector<std::vector <std::string>>> clientCommands;
  
  for (int i= 0; i<num; i++)
  {
	  if (!cmds->Get(CONTEXT_ARG i)TO_LOCAL_CHECKED ->IsArray())
	  {
		Throw(isolate,"invalid arguments");
	  }
	  Handle<Array> cmd = Handle<Array>::Cast(cmds->Get(CONTEXT_ARG i)TO_LOCAL_CHECKED);
	  std::vector <std::string> command;
	  int n = cmd->Length();
	  SlotRegion * sr = NULL;
	  for (int j= 0; j<n; j++)  {
		  if (! cmd->Get(CONTEXT_ARG j)TO_LOCAL_CHECKED ->IsString()) Throw(isolate,"invalid arguments");
		  v8::String::Utf8Value jsCmd(isolate, Handle<v8::String>::Cast(cmd->Get(CONTEXT_ARG j)TO_LOCAL_CHECKED));
		  command.push_back(*jsCmd);
		  if (j==1) {
			v8::String::Utf8Value jsKey(isolate, Handle<v8::String>::Cast(cmd->Get(CONTEXT_ARG j)TO_LOCAL_CHECKED));
			//inst = GetSSDBInstance(args.Holder(), *jsKey);
			sr = GetSSDBRegion(args.Holder(), *jsKey);
			//printf("got client from key=%s --> %p\n", *jsKey, client);
		  }
	  }
	  if (sr!=NULL) {
		clientCommands[sr].push_back(command);  
	  }
	  //commands.push_back(command);
  }  
	  
  for (auto& kv : clientCommands) {
	//printf("client=%p commands: %d\n", kv.first, kv.second.size());  
	Instance * inst = NULL;

	do {
		inst = GetSSDBInstanceFromRegion(kv.first);
		if (inst != NULL)  {
			bool res = inst->conn->pipelinedCommands(kv.second);
			if (!res) {
			   delete inst->conn;		   
			   inst->conn = NULL;
			   continue;				
			} else {
				break;
			}
		}
	} while(inst!=NULL); 
  }  
  
}

extern "C" bool LIBRARY_API attach(Isolate* isolate, v8::Local<v8::Context> &context) 
{
	v8::HandleScope handle_scope(isolate);
    Context::Scope scope(context);
	
	Handle<ObjectTemplate> ssdb = ObjectTemplate::New(isolate);
	
	ssdb->Set(v8::String::NewFromUtf8(isolate, "nodeForId")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, SSDBGetNodeForId));	
	ssdb->Set(v8::String::NewFromUtf8(isolate, "nodeId")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, SSDBGetNodeId));
	ssdb->Set(v8::String::NewFromUtf8(isolate, "connect")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, SSDBConnect));
	ssdb->Set(v8::String::NewFromUtf8(isolate, "request")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, SSDBRequest));
	ssdb->Set(v8::String::NewFromUtf8(isolate, "nodeRequest")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, SSDBNodeRequest));

	ssdb->Set(v8::String::NewFromUtf8(isolate, "pipelinedCommands")TO_LOCAL_CHECKED, FunctionTemplate::New(isolate, SSDBPipelinedCommands));
		
	
	ssdb->SetInternalFieldCount(1);  
	
	v8::Local<v8::Object> instance = ssdb->NewInstance(context).ToLocalChecked();	
	SSDBContext* ctx = new SSDBContext();
	instance->SetAlignedPointerInInternalField(0, ctx);		
	context->Global()->Set(context,v8::String::NewFromUtf8(isolate,"SSDB")TO_LOCAL_CHECKED, instance).FromJust();
	
	return true;

}

extern "C" bool LIBRARY_API init() 
{
	return true;
}
