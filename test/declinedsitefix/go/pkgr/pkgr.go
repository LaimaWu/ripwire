package pkgr

import "io"

// Info is what every packager reads.
type Info struct {
	Target string
}

// Packager is implemented once per package format.
type Packager interface {
	ConventionalFileName(info *Info) string
	Package(info *Info, w io.Writer) error
}

var registry = map[string]Packager{}

// Get looks a packager up by format name.
func Get(format string) (Packager, error) {
	return registry[format], nil
}

// WithDefaults fills the defaults in.
func WithDefaults(info *Info) *Info {
	return info
}
