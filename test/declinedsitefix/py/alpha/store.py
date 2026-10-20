class AlphaStore:
    def fetch(self, key):
        return key

    def fetch_all(self, keys, peer):
        return [peer.fetch(k) for k in keys]
