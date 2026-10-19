package app;

public class Drive {
    private Packager field;

    public void byParam(Packager p) {
        p.pack("a");
    }

    public String byLocal() {
        Packager p = Registry.get("deb");
        return p.fileName("b");
    }

    public void byField() {
        field.pack("c");
    }

    public int single(Single s) {
        return s.singleRun(1);
    }
}
