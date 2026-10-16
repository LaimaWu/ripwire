package model

import "os"

// File: an in-tree namesake of os.File (R3: a qualified outside type never types a binding by its final segment).
type File struct{}

func (f *File) Close() error { return nil }

// OpenOS returns an OUTSIDE qualified type whose final segment names the in-tree File.
func OpenOS() (*os.File, error) { return nil, nil }

// Open: an in-tree namesake of os.Open (R4).
func Open(name string) (*Config, error) { return &Config{}, nil }

// Get on Ref[T]: NearWrapper's `r.Get` is Ref.Get, neither Config's nor Other's.
func (r Ref[T]) Get(format string) *Info { return nil }

// Generic2 names its type parameter after the in-tree Config (R2: the type-parameter guard can fail).
func Generic2[Config any]() Config {
	var zero Config
	return zero
}

// Triple: grouped named results — a and b are both *Config (positions 0 and 1), o is *Other (position 2).
func Triple() (a, b *Config, o *Other) { return }

// Deb and Rpm implement Packager: an interface-typed binding proves the interface's own method, never an implementor.
type Deb struct{}

func (d *Deb) Package(info *Info) error { return nil }

type Rpm struct{}

func (r *Rpm) Package(info *Info) error { return nil }
