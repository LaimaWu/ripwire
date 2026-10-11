# fix 1 namesakes: PMid here extends PBase; ns_pb.py holds an UNRELATED PMid with a PLeaf below it.
class PBase:
    def run(self):
        raise NotImplementedError


class PMid(PBase):
    pass


class PBase2:
    pass


class PMeta(type):
    pass
