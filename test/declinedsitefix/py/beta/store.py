class BetaStore:
    def fetch(self, key):
        return None

    def fetch_all(self, keys, peer):
        return {k: peer.fetch(k) for k in keys}
