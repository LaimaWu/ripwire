package scope

import "io"

// Packager is the contract every package format implements.
type Packager interface {
	Package(w io.Writer) error
	ConventionalFileName() string
}
