function storeObjectWithTTL(obj, key, ttl) {
    var res = Redis.commandArgv('set', key, JSON.stringify(obj), 'EX', ''+ttl);
    return (res.type == Redis.REPLY_STATUS && res.string == 'OK');
}

var myFriend = {
     id: 1,
     age: 20,
     name: 'Jorge Newman'
};

storeObjectWithTTL(myFriend, 'friend:'+myFriend.id, 60);