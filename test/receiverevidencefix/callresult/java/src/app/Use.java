package app;

public class Use {
    // useVar: `var x = f()`.
    int useVar() {
        var c = Configs.make();
        return c.get();
    }

    // nearExternal: an outside callee whose result has a same-named member.
    int nearExternal() {
        var l = java.util.List.of(new Config());
        return l.get(0).get();
    }

    // nearWrapper: Optional<T> is outside the Java list; its own get() runs.
    int nearWrapper() {
        var o = Configs.find();
        return o.get().get();
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
}
