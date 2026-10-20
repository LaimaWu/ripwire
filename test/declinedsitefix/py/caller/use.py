def normalise(key):
    return key.strip()


def load(store, key):
    k = normalise(key)
    return store.fetch(k)


def load_twice(store, key):
    first = store.fetch(key)
    second = store.fetch(normalise(key))
    return first, second
