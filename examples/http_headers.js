var sockAddr = ':8210'
Http.openSocket(sockAddr);

print("Server listening on: " + sockAddr);


while(true) {
	Http.accept();
	Http.print("Status: 200 OK\r\n");
	Http.print("Content-type: text/html\r\n");
	Http.print("\r\n");
	Http.print("<pre>" + JSON.stringify(Http.getHeaders(),null,4) + "</pre>");
	Http.print("<br/>User Agent: <Br/>");
	Http.print(Http.getHeader('User-Agent'));
	Http.finish();	
}