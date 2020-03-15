var sockAddr = ':8210'
Http.openSocket(sockAddr);

print("Server listening on: " + sockAddr);

var threads = 25;

for(var i=0; i<threads; i++) {
	new Worker('./examples/worker.js');
}

