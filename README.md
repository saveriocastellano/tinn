# TINN - A Javascript application toolkit built on top of google v8/d8
TINN stands for TINN-Is-Not-NodeJS. 

TINN is a Javascript application toolkit based on the D8 Javascript shell from google-v8. TINN modifies D8 through a patch
file in order to add to it the capability of dynamically loading external C++ modules. 

Similarly to native modules in NodeJS, external modules are native shared libraries that provide additional functionalities that are exported to the Javascript context. 

Applications based on TINN are entirely written in Javascript and run in the (modified) D8 shell. 
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

## Download and setup TINN ##

### Linux users ###
Use the following commands to download TINN and the modified d8 executable:

```sh
$ wget https://github.com/saveriocastellano/tinn/archive/master.zip
$ unzip master.zip
$ cd master
$ wget https://github.com/saveriocastellano/tinn/releases/download/0.1.1/d8
```
Before continuing, make sure that the d8 executable can run on your machine (x64 machines only):
```sh
$ ./d8
```
If you get a valid shell prompt then it means that d8 is working. In any other case you need to refer to the next section to build the modified version of the d8 exeutable.

Next step is to build the TINN modules. You can choose whether to build all modules or just some of them. Once built the modules
will be under 'modules/' directory and they will be loaded automatically by d8. 

Some modules rely on external libraries that must be available on the system when building the module. For instance, the HTTP module depends on CUrl and FCGI++ libraries. 

To build all modules you need the following libraries: CUrl, FCGI++, libEvent, log4cxx, leveldb.

If you are on Debian/Ubuntu, you can install all needed libraries with this command:
```sh

$ apt-get install libfcgi0ldbl libfcgi-dev libcurl4-gnutls-dev liblog4cxx-dev libevent1-dev libleveldb-dev 
````

To build all modules just run 'make' in the 'modules/build' directory. From the TINN root directory do:

```sh
$ cd modules\build
$ make
```
To build just one module do 'make mod_name' where name is the name of the module you want to build. For instance to build only the HTTP module do:
```sh
$ make mod_http
```

Once you have the modules built you can try running one of the example scripts:
```sh
$ ./d8 examples/helloworld.js
```

### Windows users 

## Build the modified version of d8 shell ##
Only read this section if the provided d8 executable (see previous section) does not work on your machine.

Follow these steps to build the modified version of the d8 executable which supports loading external native modules:

* download and build google-v8 engine the the d8 shell executable
* apply the TINN patch to the d8 source files
* rebuild d8
* build the TINN native modules

# Linux users 
Use the commands below to download and build d8 (these steps are taken directly from google-v8 docs at https://v8.dev/docs/build):
```sh

$ git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
$ export PATH=$PATH:`pwd`/depot_tools
$ mkdir ~/v8
$ cd ~/v8
$ fetch v8
$ cd v8
$ gclient sync
$ git checkout 7.9.2
$ ./build/install-build-deps.sh
$ tools/dev/gm.py x64.release

```
Now assuming you have donwloaded TINN and uncompressed it in '~/master/' apply the patch with the following command:
```sh
$ patch -p1 < ~/tinn-master/modules/build/libs/v8_7.9/d8_v7.9.patch 
```
Now use the following command to add the '-rdynamic' link flag to the d8 makefile. This flag causes
the d8 executable to export v8 symbols when dynamically loading external modules:
```sh
$ sed -i 's/-lpthread\s-lrt/-lpthread -lrt -rdynamic/' out/x64.release/obj/d8.ninja
```

Finally, build the modified d8 executable (the following command only causes the file src/d8/d8.cc to be recompiled and then relinks the d8 executable):
```sh
$ tools/dev/gm.py x64.release
```

## Web Application ##
This example shows how to write a simple web application that replies with "Hello World!" on any request.

Because Web Application support in TINN is based on FCGI++, in order to implement a web application it is necessary to use 
a FCGI-capable Web Server like NGINX or Apache acting as a HTTP frontend. 

The following defines a script called helloworld.js that sets up a FCGI server on 8200 that replies with 'Hello World' to every request: 

```sh

var sockAddr = '127.0.0.1:8210'
Http.openSocket(sockAddr);

print("Server listening on: " + sockAddr);

Http.init();

while(true) {
        Http.accept();
        Http.print("Status: 200 OK\r\n");
        Http.print("Content-type: text/html\r\n");
        Http.print("\r\n");
        Http.print("Hello World!");
	Http.finish();	
}

```

Next thing to do is to define a location in NGINX configuration file that forwards all incoming HTTP requests to our TINN web application. Here is how a complete nginx.conf file would look like (note the 'fastcgi_pass' directive pointing to the ip and port of our example server):

```sh

events {}

http {
   server {
     location / {
        fastcgi_pass 127.0.0.1:8210;
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
  }
}

```

Now we can run the web application by doing:

```sh
$ ./d8 helloworld.js
```
And if NGINX is running on port 80 on the same machine we can send a request to our Web Application by doing:
```sh
$ curl http://127.0.0.1/
```

## Multithreaded Web Application ##

Let's see how we can modify the previous example in order to run our Web Application on multiple threads.

Basically this is just a matter of splitting helloworld.js in two parts: the first part main.js sets up the FCGI server and creates the worker threads, and the second part worker.js is the code of the worker threads that is in charge of processing the requests.

main.js
```sh

var sockAddr = '127.0.0.1:8210'
Http.openSocket(sockAddr);

print("Server listening on: " + sockAddr);

var threads = 25;

for(var i=0; i<threads; i++) {
	new Worker('worker.js');
}

```

worker.js
```sh
Http.init();

while(true) {
        Http.accept();
        Http.print("Status: 200 OK\r\n");
        Http.print("Content-type: text/html\r\n");
        Http.print("\r\n");
        Http.print("Hello World!");
	Http.finish();	
}
```


And then run the web application with:

```sh
$ ./d8 main.js
```

## Clustered Web Application ##
A clustered Web Application can be easily created by running several TINN processes (like the helloworld.js example above) on different machines (or different ports of the same machine) and then defining a cluster in the NGINX configuration file through the 'upstream' directive.

Let's say we have three HelloWorld.js running on ports 8211, 8212 and 8313. We can define our cluster in NGINX configuration using the 'upstream' directive and then direct the traffic to all nodes in the cluster:

```sh
events {}

http {
   upstream cluster_nodes {
        least_conn;
        keepalive 100;
        server 127.0.0.1:8211;
        server 127.0.0.1:8212;
        server 127.0.0.1:8213;
   }
   map $host $cluster {
        default cluster_nodes;
   }

   server {
     location / {
        fastcgi_pass $cluster;
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
  }
}

```

## Benchmark - TINN vs NodeJS
Here we show a comparison between performance of TINN vs NodeJS. 
The test was executed on Linux, using NGINX as an HTTP frontend for TINN.\
This is a single-thread test where both TINN and NodeJS are using one single worker thread to process requests.\
The test consist in sending 100k HTTP request using `ab` benchmark tool.

### Node Hello World
```sh
const http = require('http');

const hostname = '127.0.0.1';
const port = 3000;

const server = http.createServer((req, res) => {
  res.statusCode = 200;
  res.setHeader('Content-Type', 'text/plain');
  res.end('Hello World');
});

server.listen(port, hostname, () => {
  console.log(`Server running at http://${hostname}:${port}/`);
});

```

### TINN Hello World
```sh
var sockAddr = '127.0.0.1:8210'
Http.openSocket(sockAddr);

print("Server listening on: " + sockAddr);

Http.init();

while(true) {
        Http.accept();
        Http.print("Status: 200 OK\r\n");
        Http.print("Content-type: text/html\r\n");
        Http.print("\r\n");
        Http.print("Hello World!");
	Http.finish();	
}
```

This is the command used for the benchmark (given that this is  a single-thread test concurrency is set to 1):
```sh
ab -c 1 -n 100000 http://127.0.0.1/
```

### TINN result
```sh
Concurrency Level:      1
Time taken for tests:   49.832 seconds
Complete requests:      100000
Failed requests:        0
Total transferred:      14300000 bytes
HTML transferred:       1200000 bytes
Requests per second:    2006.73 [#/sec] (mean)
Time per request:       0.498 [ms] (mean)
Time per request:       0.498 [ms] (mean, across all concurrent requests)
Transfer rate:          280.24 [Kbytes/sec] received

Percentage of the requests served within a certain time (ms)
  50%      0
  66%      0
  75%      0
  80%      1
  90%      1
  95%      1
  98%      2
  99%      2
 100%     19 (longest request)

```

### NodeJS result
```sh

Concurrency Level:      1
Time taken for tests:   50.672 seconds
Complete requests:      100000
Failed requests:        0
Total transferred:      11300000 bytes
HTML transferred:       1200000 bytes
Requests per second:    1973.48 [#/sec] (mean)
Time per request:       0.507 [ms] (mean)
Time per request:       0.507 [ms] (mean, across all concurrent requests)
Transfer rate:          217.78 [Kbytes/sec] received

Percentage of the requests served within a certain time (ms)
  50%      0
  66%      0
  75%      1
  80%      1
  90%      1
  95%      1
  98%      1
  99%      2
 100%     26 (longest request)

```
