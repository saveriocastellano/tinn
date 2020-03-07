# TINN - A Javascript application toolkit built on top of google v8/d8
TINN stands for TINN-Is-Not-NodeJS. 

TINN is a Javascript application toolkit based on the D8 Javascript shell from google-v8. TINN modifies D8 through a patch
file in order to add to it the capability of dynamically loading external C++ modules. 

Similarly to native modules in NodeJS, external modules are native shared libraries that provide additional functionalities that are exported to the Javascript context. 

Applications based on TINN are entirely written in Javascript and run through the (modified) D8 shell. 
Support for threads is available through the 'Worker' Javascript class implemented in D8. 

Just like NodeJS, TINN is based on the google-v8 engine and it provides a number of native modules each one in charge of implementing a specific functionality that is "exported" to the underlying Javascript context. From an architecture viewpoint instead, TINN is the opposite of NodeJS: in TINN input/output operations are blocking and applications run on multiple cores (Worker class), whereas NodeJS was born as a single-threaded async framework.


## Features

TINN is a complete application toolkit that allows Javascript programmers to write multi-threaded apps, web applications, use databases, send HTTP requests, etc. Additional functionalities can be added easily by implementing new native modules.

Currently the following native modules are available:

* **HTTP**: module based on FCGI++ and Curl. It allows implementing a fully functional Web Server (based on Fast CGI) and provides support for sending external HTTP requests 
* **Redis**: provides support to connect to Redis servers. Two separate modules are implemented: one to connect to a single Redis server  and one to support connecting to Redis clusters
* **SSDB**: allows connecting to SSDB servers. It supports data sharding through key hashing and master/master or master/slave replication for failover and high availability.
* **LevelDB**: LevelDB database support. Allows implementing a local key-value storage based on LevelDB.
* **Logging**: Support for logging based on log4cxx library
* **OS**: implements basic functionalities to interact with the OS (opening, reading, writing files, environment variables, etc)
* **HyperLogLog**: implements the HyperLogLog algorithm
* **JS**: provides support for creating isolated Javascript execution environments (through the d8 'Realm' class)


## Compile and Install

```sh

git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export PATH=$PATH:`pwd`/depot_tools
mkdir ~/v8
cd ~/v8
fetch v8
cd v8
gclient sync
./build/install-build-deps.sh
git checkout 7.9.1
tools/dev/gm.py x64.release




```

## Web Application ##
This example shows how to write a simple web application that replies with "Hello World!" on any request.

Because Web Application support in TINN is based on FCGI++, in order to implement a web application it is necessary to use 
a FCGI-capable Web Server like NGINX or Apache acting as a HTTP frontend. 

The following defines a script called HelloWorld.js that sets up a FCGI server on 8200 that replies with 'Hello World' to every request: 

```sh

var sockAddr = '127.0.0.1:8200'
Http.openSocket(sockAddr);

print("Server listening on: " + sockAddr);

while(true) {
        Http.accept();
        Http.print("Status: 200 OK\r\n");
        Http.print("Content-type: text/html\r\n");
        Http.print("\r\n");
        Http.print("Hello World!");
	Http.finish();	
}

```

Next thing to do is to define a location in NGINX configuration file that forwards all incoming HTTP requests to our TINN web application (note the 'fastcgi_pass' directive pointing to the ip and port of our example server):

```sh

location / {
	fastcgi_pass 127.0.0.1:8201;
	fastcgi_read_timeout 255;
        fastcgi_param  GATEWAY_INTERFACE  CGI/1.1;
	fastcgi_param  SERVER_SOFTWARE    nginx;
	fastcgi_param  QUERY_STRING       $query_string;
	fastcgi_param  REQUEST_METHOD     $request_method;
	fastcgi_param  CONTENT_TYPE       $content_type;
	fastcgi_param  CONTENT_LENGTH     $content_length;
	fastcgi_param  SCRIPT_FILENAME    $document_root$fastcgi_script_name;
	fastcgi_param  SCRIPT_NAME        $fastcgi_script_name;
	fastcgi_param  REQUEST_URI        $request_uri;
	fastcgi_param  DOCUMENT_URI       $document_uri;
	fastcgi_param  DOCUMENT_ROOT      $document_root;
	fastcgi_param  SERVER_PROTOCOL    $server_protocol;
	fastcgi_param  REMOTE_ADDR        $remote_addr;
	fastcgi_param  REMOTE_PORT        $remote_port;
	fastcgi_param  SERVER_ADDR        $server_addr;
	fastcgi_param  SERVER_PORT        $server_port;
	fastcgi_param  SERVER_NAME        $server_name;
}
```
Now we can run the web application by doing:

```sh
$ ./d8 --snapshot_blob=snapshot_blob.bin HelloWorld.js
```


## Multithreaded Web Application ##
