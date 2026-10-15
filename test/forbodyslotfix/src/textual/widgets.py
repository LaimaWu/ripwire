"""Widgets that repaint themselves when their content changes."""


class Label:
    """A single line of text."""

    def __init__(self, text: str) -> None:
        self.text = text
        self.dirty = True

    def refresh(self) -> None:
        """Repaint the label: clear the dirty flag and redraw the text."""
        self.dirty = False
        print("label repaint", self.text)


class Button:
    """A clickable label with a border."""

    def __init__(self, text: str) -> None:
        self.text = text
        self.dirty = True

    def refresh(self) -> None:
        """Repaint the button: redraw the border, then the text."""
        self.dirty = False
        print("button repaint border", self.text)
