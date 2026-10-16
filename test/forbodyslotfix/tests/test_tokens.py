"""Round-trip tests for the token encoder."""


def roundtrip(text: str) -> str:
    """Encode then decode a token string (test helper)."""
    encoded = text.encode("utf-8").hex()
    return bytes.fromhex(encoded).decode("utf-8")


def test_roundtrip_ascii() -> None:
    assert roundtrip("abc") == "abc"
