import sys


class A:
    if sys.version_info >= (3,):
        def m1(self, a):
            return a
    try:
        def m2(self, a):
            return a
    except Exception:
        pass
    with open(__file__) as fh:
        def m3(self, a):
            return a
    if sys.platform:
        if sys.version_info:
            def m4(self, a):
                return a
