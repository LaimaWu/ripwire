class Widget:
    def render(self):
        return "w"

class Canvas:
    def render(self):
        return "c"

def draw_widget():
    w = Widget()
    return w.render()

def draw_canvas():
    c = Canvas()
    return c.render()

def draw_both():
    w = Widget()
    c = Canvas()
    return w.render() + c.render()

def draw_mixed(thing):
    w = Widget()
    w.render()
    return thing.render()
