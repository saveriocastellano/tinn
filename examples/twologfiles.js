
Log.init("examples/log_twologfiles.properties");
//print("cccio=" + process.ciccio);
var dt1= Date.now();
Log.info("this will be logged to mylogger.log", "mylogger");
var dt2= Date.now();
Log.info("this will be logged to mylogger.log", "mylogger");
var dt3= Date.now();
Log.info("this will be logged to mylogger.log", "mylogger");
var dt4= Date.now();

print("first=" + (dt2-dt1));
print("first=" + (dt3-dt2));
print("first=" + (dt4-dt3));

Log.info("this will be logged to tinn.log");