package app;

public class Use {
    // useVar: `var x = f()`.
    int useVar() {
        var c = Configs.make();
        return c.get();
    }

    // nearExternal: an outside callee (List.of, beside the in-tree Configs.of) whose result has a same-named member.
    int nearExternal() {
        var l = java.util.List.of(new Config());
        l.get(0);
        return 0;
    }

    // nearWrapper: Optional<T> is outside the Java list; its own get() runs.
    int nearWrapper() {
        var o = Configs.find();
        o.get();
        return 0;
    }

    // nearGeneric: a type-parameter result.
    int nearGeneric() {
        var g = Configs.<Config>create();
        return g.get();
    }

    // useWritten (twin): the same call with the type written.
    int useWritten() {
        Config c = Configs.make();
        return c.get();
    }

    // nearTypeParamNamed: a type parameter named after the in-tree Config, instantiated with Other.
    int nearTypeParamNamed() {
        var g = Configs.<Other>create2();
        return g.get();
    }
}
