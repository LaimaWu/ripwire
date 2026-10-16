package model

type Info struct{}

type Config struct{}

func (c *Config) Get(format string) *Info { return nil }

func (c *Config) Close() error { return nil }

type Other struct{}

func (o *Other) Get(format string) *Info { return nil }

func (o *Other) Close() error { return nil }

type Packager interface {
	Package(info *Info) error
}

type Alias = Config

type Ref[T any] struct{ v T }

// ParseFile names its results: `(config Config, err error)`.
func ParseFile(path string) (config Config, err error) { return }

func Load(path string) (*Config, error) { return &Config{}, nil }

func Make() *Config { return &Config{} }

func Pair() (*Config, *Other) { return &Config{}, &Other{} }

func Lookup(format string) (Packager, error) { return nil, nil }

func MakeAlias() Alias { return Alias{} }

func Wrap() Ref[Config] { return Ref[Config]{} }

func Generic[T any]() T {
	var zero T
	return zero
}

type Builder struct{}

func (b *Builder) Build() *Config { return &Config{} }

type Maker struct{}

func (m *Maker) Build() *Other { return &Other{} }

// Get: a package-level func beside the method Config.Get (nfpm's `nfpm.Get` beside `(*Config).Get`).
func Get(format string) (Packager, error) { return nil, nil }
