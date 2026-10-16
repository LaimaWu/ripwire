package app;

import java.util.Optional;

public final class Configs {
    public static Config make() { return new Config(); }
    public static Optional<Config> find() { return Optional.of(new Config()); }
    public static <T> T create() { return null; }
}
