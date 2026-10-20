class Cache {
    String lookup(String k) { return k; }
}
class Registry {
    String lookup(String k) { return k + "!"; }
}
class Store {
    String fromCache(Cache c) { return c.lookup("a"); }
    String fromRegistry(Registry r) { return r.lookup("b"); }
    String fromBoth(Cache c, Registry r) { return c.lookup("a") + r.lookup("b"); }
}
