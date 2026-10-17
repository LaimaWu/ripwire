def g(x, y=1):
    return x + y


def g2(x, *, y=1):
    return x + y


def deco(fn):
    return fn


@deco
def k(a, b):
    return a + b
