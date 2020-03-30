# TINN - A Javascript application toolkit built on top of google v8
TINN stands for TINN-Is-Not-NodeJS. 

TINN is a Javascript application toolkit based on the google V8 javascript engine.  
TINN consists of a javascript interpreter and shell derived from the D8 shell of V8 and native modules that
are dynamically loaded and provide additional functionalities that are exported to the Javascript context. 

Applications based on TINN are entirely written in Javascript and run in the TINN shell. 
Support for threads is available through the 'Worker' Javascript class from D8. 

Just like NodeJS, TINN is based on the google-v8 engine and it provides a number of native modules each one in charge of implementing a specific functionality that is "exported" to the underlying Javascript context. From an architecture viewpoint instead, TINN is the opposite of NodeJS: in TINN input/output operations are blocking and applications run on multiple cores (through the Worker class), whereas NodeJS was born as a single-threaded async framework.

# Why TINN ##
TINN was started as an experiment and it is not intended to be a competitor to NodeJS. However it can be an alternative to it.  
While NodeJS has many advantages without a doubt, as it is a very mature and complete framework with a huge community, in order to better describe TINN a good start is to list some of its benefits with respect to NodeJS: 

- TINN has a shorter learning curve. No need to learn about promises and async stuff, and it's not necessary to learn about modules and `require`, one can simply load scripts in the current scope using the build-in `load` function
- By leveraging on the `Worker` class, TINN makes it very easy to implement multi-threaded apps 
- TINN is a javascript shell, javascript interpreter and package manager in a single executable. No need to install other tools or external package managers
- people familiar with `PHP` will feel immediataly confortable choosing TINN as a web framework: TINN Web framework has many similarities to PHP 
- TINN seems to outperform NodeJS. See the benchmarks `


## Features

TINN is a complete application toolkit that allows Javascript programmers to write multi-threaded apps, web applications, databases apps, etc. New functionalities can be added easily by implementing additional native modules.

Currently the following native modules are available:

* **HTTP**: module based on FCGI++ and Curl. It allows implementing a fully functional Web Server (based on Fast CGI) and provides support for sending external HTTP requests 
* **Redis**: provides support to connect to Redis servers. Two separate modules are implemented: one to connect to a single Redis server  and one to support connecting to Redis clusters
* **SSDB**: allows connecting to SSDB servers. It supports data sharding through key hashing and master/master or master/slave replication for failover and high availability.
* **LevelDB**: LevelDB database support. Allows implementing a local key-value storage based on LevelDB.
* **Logging**: Support for logging based on log4cxx library
* **HyperLogLog**: implements the HyperLogLog algorithm
* **JS**: provides support for creating isolated Javascript execution environments (through the d8 'Realm' class)

In addition to the above modules, the following features are built-in the TINN shell and exported to the javascript context:

- OS functions (opening, reading and writing files, environment variables, etc)
- require function to load javascript modules in the same fashion as  Node's require function
- package manager supporting modules version and dependencies (similar to npm)
- support for building and loading native modules

## Documentation ##
Refer to the [wiki](https://github.com/saveriocastellano/tinn/wiki) page.  


## Download and setup TINN ##

### Linux users ###
Use the following commands to download TINN and the pre-built TINN shell executable:

```sh
$ wget https://github.com/saveriocastellano/tinn/archive/master.zip
$ unzip master.zip
$ cd master
$ wget https://github.com/saveriocastellano/tinn/releases/download/0.0.1/tinn_shell_0.0.1_x64.tgz
$ tar -zxvf tinn_shell_0.0.1_x64.tgz
```
Before continuing, make sure that the TINN executable can run on your machine (x64 machines only):
```sh
$ ./tinn 
```
If you get a valid shell prompt then it means that TINN is working. In any other case you need to refer to the next section to build the TINN shell executable.

Next step is to build the TINN modules. You can choose whether to build all modules or just some of them. Once built, the modules
will be under 'modules/' directory and they will be loaded automatically by TINN. 

Some modules rely on external libraries that must be available on the system when building the module. For instance, the HTTP module depends on CURL and FCGI++ libraries. 

To build all modules you need the following libraries: CURL, FCGI++, libEvent, log4cxx, leveldb.

If you are on Debian/Ubuntu, you can install all needed libraries with this command:
```sh

$ apt-get install libfcgi0ldbl libfcgi-dev libcurl4-gnutls-dev liblog4cxx-dev libevent1-dev libleveldb-dev 
````

To build all modules run the 'tinn build' command in the directory where you uncompressed TINN:

```sh
$ ./tinn build
```
To build just one module do 'tinn build name' where `name` is the name of the module you want to build. For instance to build only the HTTP module do:
```sh
$ ./tinn build http
```

Once you have the modules built you can try running one of the example scripts:
```sh
$ ./tinn examples/helloworld.js
```

### Windows users 
A binary version of TINN is available for x64 machines:  


[tinn_windows_x64_0.0.1.zip (25 MB)](https://github.com/saveriocastellano/tinn/releases/download/0.0.1/tinn_windows_x64_0.0.1.zip)  


Unzip the content of tinn_windows_x64_0.0.1.zip in a directory. You can then run scripts by using the tinn.exe executable:
```sh
tinn.exe examples\helloworld.js
```


## Build the TINN shell ##
Only read this section if the provided TINN executable (see previous section) does not work on your machine.

Follow these steps to build the TINN executable:

* download and build the google v8 engine 
* use the `tinn_build.sh` script in `build` to build the TINN executable on top of v8 

After building the TINN executable, TINN native modules can be built as already described in the previous section.

### Linux users 
Use the commands below to download and build v8 (these steps are taken directly from google-v8 docs at https://v8.dev/docs/build):
```sh
$ git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
$ export PATH=$PATH:`pwd`/depot_tools
$ fetch v8
$ cd v8
$ gclient sync
$ git checkout 7.9.2
$ ./build/install-build-deps.sh
$ tools/dev/gm.py x64.release

```
Now from the TINN directory run the `tinn.sh` script in `build/` and pass it the directory where v8 is from the previous step:
```sh
$ ./build/tinn.sh /opt/v8
```
In the above command you need to change `/opt/v8/` to wherever you put v8 in your sytem.  
If the script was successful then you will have the following files in the TINN directory:

- tinn 
- natives_blob.bin
- snapshot_blob.bin

You can then build all native modules by doing:
```sh
$ ./tinn build
```

## Web Application ##
This example shows how to write a simple web application that replies with "Hello World!" on any request.

Because Web Application support in TINN is based on FCGI++, in order to implement a web application it is necessary to use 
a FCGI-capable Web Server like NGINX or Apache acting as a HTTP frontend. 

The following defines a script called helloworld.js that sets up a FCGI server on 8200 that replies with 'Hello World' to every request: 

```sh

var sockAddr = ':8210'
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

var sockAddr = ':8210'
Http.openSocket(sockAddr);

print("Server listening on: " + sockAddr);

var threads = 25;

for(var i=0; i<threads; i++) {
	new Worker('worker.js');
}

```

worker.js
```sh
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
Here we show a performance comparison between TINN vs NodeJS. 
The test was executed on Linux, using NGINX as the HTTP frontend for TINN.\
This is a single-thread test where both TINN and NodeJS are using one single worker to process requests.\
The test consists in sending 100k HTTP requests using the `ab` benchmark tool.

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
var sockAddr = ':8210'
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
