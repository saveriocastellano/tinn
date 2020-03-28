#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include "SSDB_client.h"
#include <unistd.h>
int main(int argc, char **argv){
	const char *ip = (argc >= 2)? argv[1] : "192.168.193.91";
	int port = (argc >= 3)? atoi(argv[2]) : 7501;

	ssdb::Client *client = ssdb::Client::connect(ip, port);
	if(client == NULL){
		printf("fail to connect to server!\n");
		return 0;
	}
	
	std::vector<std::string> req1;
	std::vector<std::string> req2;
	std::vector<std::string> req3;
	req1.push_back("set");
	req1.push_back("mykey11");
	req1.push_back("myval11z");
	
	req2.push_back("set");
	req2.push_back("mykey22");
	req2.push_back("myval22zy");

	req3.push_back("set");
	req3.push_back("mykey33");
	req3.push_back("myval33zy");

	printf("call pipelined request\n");
	client->pipelinedRequest(req1);
	client->pipelinedRequest(req2);
	client->pipelinedRequest(req3);
	printf("called pipelined request\n");
	
	const std::vector<std::string> *resp;
	printf("call pipelined read\n");

	bool res = client->pipelineExec();
	printf("called pipelined read\n");

	if (res) {
		printf("got response...\n");
	} else {
		
		printf("NULL response\n");
	}
	
	
	ssdb::Status s;
	std::string val;
	std::vector<std::string> vals;

	/*while(true) {
		// set and get
		if (!client) {
			printf("reconnecting\n");
			client  = ssdb::Client::connect(ip, port);
		} else {
			s = client->set("k", "hello ssdb!");
			if(s.ok()){
				printf("k = hello ssdb!\n");
			}else{
				printf("error!\n");
				delete client;
				client  = ssdb::Client::connect(ip, port);
				if (client == NULL) {
					printf("failed to reconnect\n");
					
				}
			}
		}
		sleep(1);
	}
	
	s = client->get("k", &val);
	printf("length: %d\n", (int)val.size());

	// qpush, qslice, qpop
	s = client->qpush("k", "hello1!");
	if(s.ok()){
		printf("qpush k = hello1!\n");
	}else{
		printf("error!\n");
	}
	s = client->qpush("k", "hello2!");
	if(s.ok()){
		printf("qpush k = hello2!\n");
	}else{
		printf("error!\n");
	}
	s = client->qslice("k", 0, 1, &vals);
	if(s.ok()){
		printf("qslice 0 1\n");
		for(int i = 0; i < (int)vals.size(); i++){
			printf("    %d %s\n", i, vals[i].c_str());
		}
	}else{
		printf("error!\n");
	}
	s = client->qpop("k", &val);
	if(s.ok()){
		printf("qpop k = %s\n", val.c_str());
	}else{
		printf("error!\n");
	}
	s = client->qpop("k", &val);
	if(s.ok()){
		printf("qpop k = %s\n", val.c_str());
	}else{
		printf("error!\n");
	}
    */
	delete client;
	return 0;
}
