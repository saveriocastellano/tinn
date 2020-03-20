function storeObjectWithTTL(obj, key, ttl) {
    var res = Redis.commandArgv('set', key, JSON.stringify(obj), 'EX', ''+ttl);
	print("res=" + JSON.stringify(res));
	print("cmd=" + ['set', key, JSON.stringify(obj), 'EX', ''+ttl].join(' ' ));
    return (res.type == Redis.REPLY_STATUS && res.string == 'OK');
}

var myFriend = {
     id: 1,
     age: 20,
     name: 'Jorge Newman'
};

print("connect: " +Redis.connect("127.0.0.1", 6379));

print(storeObjectWithTTL(myFriend, 'friend:'+myFriend.id, 60));