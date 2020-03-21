var hll = HyperLogLog.addToNew(10, "string #1");
hll = HyperLogLog.add(hll,  "string #2")
hll = HyperLogLog.add(hll,  "string #3")
hll = HyperLogLog.add(hll,  "string #4")
hll = HyperLogLog.add(hll,  "string #5")
hll = HyperLogLog.add(hll,  "string #6")
hll = HyperLogLog.add(hll,  "string #7")
hll = HyperLogLog.add(hll,  "string #8")
hll = HyperLogLog.add(hll,  "string #9")
hll = HyperLogLog.add(hll,  "string #10");

var card = HyperLogLog.getCardinality(hll);

print("the cardinality after adding 10 different string values is: " + card);