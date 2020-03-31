# TINN - A Javascript application toolkit built on top of google v8
TINN stands for TINN-Is-Not-NodeJS. 

TINN is a Javascript application toolkit based on the google V8 javascript engine.  
TINN consists of a javascript interpreter and shell derived from the D8 shell of V8 and native modules that
are dynamically loaded and provide additional functionalities that are exported to the Javascript context. 

Applications based on TINN are entirely written in Javascript and run in the TINN shell. 
Support for threads is available through the 'Worker' Javascript class from D8. 

Just like NodeJS, TINN is based on the google-v8 engine and it provides a number of native modules each one in charge of implementing a specific functionality that is "exported" to the underlying Javascript context. From an architecture viewpoint instead, TINN is the opposite of NodeJS: in TINN input/output operations are blocking and applications run on multiple cores (through the Worker class), whereas NodeJS was born as a single-threaded async framework.

## Why TINN ##
TINN was started as an experiment and it is not intended to be a competitor to NodeJS. However it can be an alternative to it.  
While NodeJS has many advantages without a doubt, as it is a very mature and complete framework with a huge community, in order to better describe TINN a good start is to list some of its benefits with respect to NodeJS: 

- TINN has a shorter learning curve. No need to learn about promises and async stuff, and it's not necessary to learn about modules and `require` (although TINN supports `require` and javascript modules just like NodeJS): one can simply load scripts in the current scope using the build-in `load` function
- By leveraging on the `Worker` class, TINN makes it very easy to implement multi-threaded apps 
- TINN is a javascript shell, javascript interpreter and package manager in a single executable. No need to install other tools or external package managers
- people familiar with `PHP` will feel immediataly confortable choosing TINN as a web framework: [TINN Web](https://github.com/saveriocastellano/tinn/wiki/TINN-Web) framework has many similarities to PHP 
- TINN seems to outperform NodeJS. See the [benchmarks](https://github.com/saveriocastellano/tinn#benchmark---tinn-vs-nodejs) `


## Features ##

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

**NOTE FOR WINDOWS USERS**: if you downloaded the TINN package (tinn_windows_x64_0.0.1.zip) you don't need to deal with configuring NGINX, the package contains a working NGINX server and the `Http.openSocket` automatically takes care of configuring and starting NGINX to point to port specified in the `Http.openSocket` call

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

## Benchmarks - TINN vs NodeJS ##
Here we show some tests that have been made to compare the performance between TINN vs NodeJS. 
The tests were executed on Linux on a quad core Intel(R) Xeon(R) CPU E5-2670 0 @ 2.60GHz, using NGINX as the HTTP frontend for TINN.\
&nbsp;  
### Benchmark1: Hello World in HTTP ###
This is a single-thread test where both TINN and NodeJS are using one single worker to process requests.\
The test consists in sending 100k HTTP requests using the `ab` benchmark tool.

#### NodeJS Hello World ####
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

#### TINN Hello World ####
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

#### NodeJS result ####
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

#### TINN result ####
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

#### Conclusion ####
From the results of this test we see that for the simple `Hello World` case the performance of TINN is very similar to NodeJS and only slightly better.  
Let's introduce some I/O and move on to a more complex test case...


### Benchmark2: Generate a random file and send its content in the response ###
In this second test we will use some code that will do the following at every request:
* generate a random string of 108kb
* create a file with a random name and write the generated string into it 
* read the content of the file into a new string
* send the response using the content of the new string

In the benchmark we will send 2000 requests and use a concurrency set to 50: 
```sh
ab -c 50 -n 2000 http://127.0.0.1/
```


#### NodeJS code ####
```sh
const hostname = '127.0.0.1';
const port = 3000;

//server.js
var http = require('http');    
var server = http.createServer(handler);
var fs = require('fs');

function handler(request, response) {
    response.writeHead(200, {'Content-Type': 'text/plain'});

    //generate a random filename
    do{fname = (1 + Math.floor(Math.random()*99999999))+'.txt';
    } while(fs.existsSync(fname));

    //generate a random string of 108kb
    var payload="";
    for(i=0;i<108000;i++)
    {
        n=Math.floor(65 + (Math.random()*(122-65)) );
        payload+=String.fromCharCode(n);
    }

    //write the string to disk in async manner
    fs.writeFile(fname, payload, function(err) {
            if (err) console.log(err);

            //read the string back from disk in async manner
            fs.readFile(fname, function (err, data) {
                if (err) console.log(err);
                response.end(data); //write the string back on the response stream
            });  
        }
    );
}

server.listen(port, hostname, () => {
  console.log(`Server running at http://${hostname}:${port}/`);
});
```

#### TINN code ####
```sh

var workerCode = `
while(true) {
	Http.accept();
	
    //generate a random filename
    do{fname = (1 + Math.floor(Math.random()*99999999))+'.txt';
    } while(os.isFileAndReadable(fname));

    //generate a random string of 108kb
    var payload="";
    for(i=0;i<108000;i++)
    {
        n=Math.floor(65 + (Math.random()*(122-65)) );
        payload+=String.fromCharCode(n);
    }
    
    os.writeFile(fname, payload);
	var data = os.readFile(fname);

	Http.print('Status: 200 OK\\r\\n');
	Http.print('Content-type: text/plain\\r\\n');
	Http.print('\\r\\n');
	Http.print(data);
	Http.finish();	
}
`;


var sockAddr = ':8210'
Http.openSocket(sockAddr);

print("Server listening on: " + sockAddr);

var threads = 50;

for(var i=0; i<threads; i++) {
	new Worker(workerCode, {type:'string'});
}


```
#### NodeJS result #### 

```sh
Concurrency Level:      50
Time taken for tests:   59.583 seconds
Complete requests:      2000
Failed requests:        37
   (Connect: 0, Receive: 0, Length: 37, Exceptions: 0)
Total transferred:      214356400 bytes
HTML transferred:       214154400 bytes
Requests per second:    33.57 [#/sec] (mean)
Time per request:       1489.572 [ms] (mean)
Time per request:       29.791 [ms] (mean, across all concurrent requests)
Transfer rate:          3513.30 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.7      0       9
Processing:   455 1485 197.6   1464    2373
Waiting:      391 1316 229.6   1304    1885
Total:        455 1486 197.6   1464    2373

Percentage of the requests served within a certain time (ms)
  50%   1464
  66%   1572
  75%   1620
  80%   1626
  90%   1745
  95%   1804
  98%   1848
  99%   1925
 100%   2373 (longest request)

```

#### TINN result ####
```sh
Concurrency Level:      50
Time taken for tests:   4.924 seconds
Complete requests:      2000
Failed requests:        0
Total transferred:      216264000 bytes
HTML transferred:       216000000 bytes
Requests per second:    406.20 [#/sec] (mean)
Time per request:       123.093 [ms] (mean)
Time per request:       2.462 [ms] (mean, across all concurrent requests)
Transfer rate:          42893.27 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    3   7.5      0      56
Processing:     8  118  61.9    106     514
Waiting:        0  104  62.9     92     512
Total:          8  122  61.8    111     516

Percentage of the requests served within a certain time (ms)
  50%    111
  66%    136
  75%    152
  80%    163
  90%    196
  95%    240
  98%    308
  99%    331
 100%    516 (longest request)

```

From these results there seem to be a huge difference. But the NodeJS script is using a single core. Let's rewrite it using the `cluster` module so that it can use multiple cores:

#### NodeJS code (multicore) ####
```sh
const cluster = require('cluster');
const http = require('http');
const numCPUs = require('os').cpus().length; //number of CPUS
var fs = require('fs');

if (cluster.isMaster) {
  // Fork workers.
  for (var i = 0; i < numCPUs; i++) {
    cluster.fork();    //creating child process
  }

  //on exit of cluster
  cluster.on('exit', (worker, code, signal) => {
      if (signal) {
        console.log(`worker was killed by signal: ${signal}`);
      } else if (code !== 0) {
        console.log(`worker exited with error code: ${code}`);
      } else {
        console.log('worker success!');
      }
  });
} else {
  // Workers can share any TCP connection
  // In this case it is an HTTP server
  http.createServer((req, response) => {
    response.writeHead(200, {'Content-Type': 'text/plain'});

    //generate a random filename
    do{fname = (1 + Math.floor(Math.random()*99999999))+'.txt';
    } while(fs.existsSync(fname));

    //generate a random string of 108kb
    var payload="";
    for(i=0;i<108000;i++)
    {
        n=Math.floor(65 + (Math.random()*(122-65)) );
        payload+=String.fromCharCode(n);
    }

    //write the string to disk in async manner
    fs.writeFile(fname, payload, function(err) {
            if (err) console.log(err);

            //read the string back from disk in async manner
            fs.readFile(fname, function (err, data) {
                if (err) console.log(err);
                response.end(data); //write the string back on the response stream
            });  
        }
    );
	  
	  
  }).listen(3000);
}
```
Executing the benchmark again with the multicore script yelds the following result:


#### Node result ####
```sh
Concurrency Level:      50
Time taken for tests:   6.734 seconds
Complete requests:      2000
Failed requests:        20
   (Connect: 0, Receive: 0, Length: 20, Exceptions: 0)
Total transferred:      214513040 bytes
HTML transferred:       214311040 bytes
Requests per second:    297.00 [#/sec] (mean)
Time per request:       168.353 [ms] (mean)
Time per request:       3.367 [ms] (mean, across all concurrent requests)
Transfer rate:          31108.15 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.5      0       8
Processing:    62  167  45.4    151     436
Waiting:       62  155  42.7    140     428
Total:         62  167  45.5    151     439

Percentage of the requests served within a certain time (ms)
  50%    151
  66%    166
  75%    181
  80%    194
  90%    237
  95%    263
  98%    288
  99%    320
 100%    439 (longest request)
```


#### Conclusion #### 
In this second bencmark the time taken by TINN to reply to the 2000 requests was 4.9 seconds, while NodeJS multicore took 6.7 seconds.  
The performance of TINN for this test is 25% better than NodeJS.

### Benchmark 3: Redis hmset/hgetall ###
The code for this test at every request does the following:

* sets a hash in redis with 100 keys whose values are strings of 1000 characters
* reads all keys in the hash

#### TINN code ####

```sh
var workerCode = `

Redis.connect('127.0.0.1', 6379);

while(true) {
	Http.accept();
	
	var values = [];
	for (var k=0; k<100; k++) {
		//generate a random string of 108kb
		var payload="";
		for(i=0;i<1000;i++)
		{
			n=Math.floor(65 + (Math.random()*(122-65)) );
			payload+=String.fromCharCode(n);
		}
		var key = 'key'+(1 + Math.floor(Math.random()*99999999));
		values.push(key);
		values.push(payload)
	}
	Redis.command("del mykey");
	Redis.command('hmset mykey ' + values.join(' '));
	var res = Redis.command('hgetall mykey');
		
		
	Http.print('Status: 200 OK\\r\\n');
	Http.print('Content-type: text/plain\\r\\n');
	Http.print('\\r\\n');
	Http.print('done');
	Http.finish();	
}
`;


var sockAddr = ':8210'
Http.openSocket(sockAddr);

print("Server listening on: " + sockAddr);

var threads = 20;

for(var i=0; i<threads; i++) {
	new Worker(workerCode, {type:'string'});
}
```
#### NodeJS code ####

```sh
const cluster = require('cluster');
const http = require('http');
const numCPUs = require('os').cpus().length; //number of CPUS
var fs = require('fs');

if (cluster.isMaster) {
  // Fork workers.
  for (var i = 0; i < numCPUs; i++) {
    cluster.fork();    //creating child process
  }

  //on exit of cluster
  cluster.on('exit', (worker, code, signal) => {
      if (signal) {
        console.log(`worker was killed by signal: ${signal}`);
      } else if (code !== 0) {
        console.log(`worker exited with error code: ${code}`);
      } else {
        console.log('worker success!');
      }
  });
} else {
	
	var redis = require('redis');
	var client = redis.createClient();

	client.on('connect', function() {
		console.log('connected to redis');
	});

    http.createServer((req, response) => {
		response.writeHead(200, {'Content-Type': 'text/plain'});

		var values = [];
		for (var k=0; k<100; k++) {
			var payload="";
			for(i=0;i<1000;i++)
			{
				n=Math.floor(65 + (Math.random()*(122-65)) );
				payload+=String.fromCharCode(n);
			}
			var key = 'key'+(1 + Math.floor(Math.random()*99999999));
			values.push(key);
			values.push(payload)
		}
		client.del("mykey");
		client.hmset.apply(client, ['mykey'].concat(values));
		client.hgetall('mykey', function(err, object) {
			response.end('done');
			
		});		
		

    }).listen(3000);
}
```

#### TINN result ####
```sh
Concurrency Level:      50
Time taken for tests:   7.907 seconds
Complete requests:      2000
Failed requests:        0
Total transferred:      272000 bytes
HTML transferred:       8000 bytes
Requests per second:    252.93 [#/sec] (mean)
Time per request:       197.685 [ms] (mean)
Time per request:       3.954 [ms] (mean, across all concurrent requests)
Transfer rate:          33.59 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.6      0      21
Processing:    87  196  45.6    187     425
Waiting:       87  196  45.5    187     425
Total:         89  196  45.6    187     426

Percentage of the requests served within a certain time (ms)
  50%    187
  66%    206
  75%    219
  80%    226
  90%    254
  95%    291
  98%    319
  99%    340
 100%    426 (longest request)
```
#### NodeJS Result ####
```sh
Concurrency Level:      50
Time taken for tests:   8.408 seconds
Complete requests:      2000
Failed requests:        0
Total transferred:      210000 bytes
HTML transferred:       8000 bytes
Requests per second:    237.86 [#/sec] (mean)
Time per request:       210.212 [ms] (mean)
Time per request:       4.204 [ms] (mean, across all concurrent requests)
Transfer rate:          24.39 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.3      0       3
Processing:    43  208  33.8    203     472
Waiting:       26  193  31.3    188     456
Total:         45  208  33.8    203     473

Percentage of the requests served within a certain time (ms)
  50%    203
  66%    215
  75%    223
  80%    229
  90%    250
  95%    271
  98%    298
  99%    317
 100%    473 (longest request)

```

#### Conclusion #### 
The results of this test are similar: TINN took 6% less than NodeJS to execute the test.


### Benchmark 4: serving static files ###
This test consists in sending 100k requests with concurrency set to 50 to download the following HTML file:
```sh
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<title></title>
</head>
<body>
Hello World
</body>
</html>

```
The following command is used:
```sh
 ab -n 100000 -c 50 http://127.0.0.1:5000/index.html
```

#### TINN code ####
```sh
var workerCode = `

Redis.connect('127.0.0.1', 6379);

while(true) {
	Http.accept();
	
	var txt = os.readFile('.'+Http.getParam('SCRIPT_NAME'));
		
	Http.print('Status: 200 OK\\r\\n');
	Http.print('Content-type: text/html\\r\\n');
	Http.print('\\r\\n');
	Http.print(txt);
	Http.finish();	
}
`;


var sockAddr = ':8210'
Http.openSocket(sockAddr);

print("Server listening on: " + sockAddr);

var threads = 20;

for(var i=0; i<threads; i++) {
	new Worker(workerCode, {type:'string'});
}
```

#### NodeJS code ####
```sh
const cluster = require('cluster');
const numCPUs = require('os').cpus().length; //number of CPUS
var fs = require('fs');

if (cluster.isMaster) {
  // Fork workers.
  for (var i = 0; i < numCPUs; i++) {
    cluster.fork();    //creating child process
  }

  //on exit of cluster
  cluster.on('exit', (worker, code, signal) => {
      if (signal) {
        console.log(`worker was killed by signal: ${signal}`);
      } else if (code !== 0) {
        console.log(`worker exited with error code: ${code}`);
      } else {
        console.log('worker success!');
      }
  });
} else {
	

var http = require('http');

var nStatic = require('node-static');

var fileServer = new nStatic.Server('./');

http.createServer(function (req, res) {
    
    fileServer.serve(req, res);

}).listen(5000);

}

```

#### TINN result ####
```sh
Concurrency Level:      50
Time taken for tests:   6.784 seconds
Complete requests:      100000
Failed requests:        0
Non-2xx responses:      100000
Total transferred:      33700000 bytes
HTML transferred:       17800000 bytes
Requests per second:    14739.59 [#/sec] (mean)
Time per request:       3.392 [ms] (mean)
Time per request:       0.068 [ms] (mean, across all concurrent requests)
Transfer rate:          4850.82 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    1   0.5      1      15
Processing:     0    2   1.1      2      17
Waiting:        0    2   1.0      2      16
Total:          0    3   1.3      3      22

Percentage of the requests served within a certain time (ms)
  50%      3
  66%      3
  75%      4
  80%      4
  90%      4
  95%      6
  98%      8
  99%      9
 100%     22 (longest request)
```

#### Node result ####

```sh
Concurrency Level:      50
Time taken for tests:   25.466 seconds
Complete requests:      100000
Failed requests:        0
Total transferred:      37900000 bytes
HTML transferred:       12100000 bytes
Requests per second:    3926.82 [#/sec] (mean)
Time per request:       12.733 [ms] (mean)
Time per request:       0.255 [ms] (mean, across all concurrent requests)
Transfer rate:          1453.38 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.3      0      14
Processing:     1   13   5.5     11     188
Waiting:        1   11   4.6     10     173
Total:          2   13   5.6     11     200

Percentage of the requests served within a certain time (ms)
  50%     11
  66%     13
  75%     14
  80%     16
  90%     19
  95%     22
  98%     26
  99%     30
 100%    200 (longest request)

```
#### Conclusion ####
This test took 6.8 seconds to execute with TINN and 12.7 seconds with NodeJS, so basically TINN was nearly twice as fast.
