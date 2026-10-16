package app

// useVal: `val x = f()` with a declared return type.
fun useVal(): Int {
    val c = makeConfig()
    return c.get()
}

// useVar: `var x = f()`, reassigned with the same type.
fun useVar(): Int {
    var c = makeConfig()
    c = makeConfig()
    return c.get()
}

// nearInferred: an expression-bodied function with no written return type.
fun nearInferred(): Int {
    val c = makeLazy()
    return c.get()
}

// useWritten (twin): the same call with the type written.
fun useWritten(): Int {
    val c: Config = makeConfig()
    return c.get()
}
