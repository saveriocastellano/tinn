var sockAddr = ':8210'
Http.openSocket(sockAddr);

print("Server listening on: " + sockAddr);


function getHeaders() {
	var params = Http.getParams();
	var headers = {};
	var HTTP_ = 'HTTP_';
	for (var i=0; i<params.length; i++) {
		var eq =  params[i].indexOf('=');
		var name = params[i].substring(0, eq);
		var st = name.indexOf(HTTP_);
		if (st==0) {
			name = name.substring(HTTP_.length).replace(/_/g,"-");
			var hdr = '';
			for (var j=0; j<name.length; j++) {
				var c = name.charAt(j)
				hdr += j==0 ? c : (name.charAt(j-1)=='-' ? c : c.toLowerCase()); 
			}		
			headers[hdr] = params[i].substring(eq+1);
		}
	}
	return headers;
}

function getHeader(name){ 
	name = 'HTTP_' + name.replace('-','_').toUpperCase();
	return Http.getParam(name);
}


while(true) {
	Http.accept();
	getHeaders();
	Http.print("Status: 200 OK\r\n");
	Http.print("Content-type: text/html\r\n");
	Http.print("\r\n");
	Http.print("<pre>" + JSON.stringify(getHeaders(),null,4) + "</pre>");
	Http.print("<br/>User Agent: <Br/>");
	Http.print(getHeader('User-Agent'));
	Http.finish();	
}