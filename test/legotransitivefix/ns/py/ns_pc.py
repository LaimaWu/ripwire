from typing import Generic

from ns_pa import PBase, PBase2, PMeta, PMid


class PTrue(PMid):
    pass


class POuter:
    class PChanged:
        pass


class PInner(POuter.PChanged):
    pass


class PKey:
    pass


class PBox(Generic[PKey]):
    pass


class PSub(PBase2[int]):
    pass


class PWithMeta(PBase, metaclass=PMeta):
    pass
