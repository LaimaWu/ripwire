package app

class Config {
    fun get(): Int = 0
}

class Other {
    fun get(): Int = 1
}

fun makeConfig(): Config = Config()

fun makeLazy() = Config()
