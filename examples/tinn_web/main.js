
var web = require("tinn_web");

print("Server root: " + web.getServerRoot());

var sockAddr = ':8210'
Http.openSocket(sockAddr);

print("Server listening on: " + sockAddr);

/*
web.setErrorHandler(function(e) {
	print("error handler called "+ e.message + ' ' + e.stack);
	if (e instanceof this.PageNotFound) {
		this.response("pagina non trovata: " + this.getPagePath());
	}
});*/

web.addRequest('Json', function() {
	print("throw scemo2");
	throw new Error("scemo");
	web.response("ciao json<Br>");
});


web.addRequest('JsonTest', function() {
	print("throw scemo");
	//throw new Error("scemo");
	web.response("ciao json test<br>");
});

while(true) {
	Http.accept();
	web.handleRequest();
	Http.finish();	
}