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