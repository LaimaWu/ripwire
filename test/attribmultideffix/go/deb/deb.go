package deb

import "example.com/pk/files"

type Info struct{ Name string }

func (i *Info) Get(key string) string { return i.Name + key }

func Package(i *Info) []string {
	_ = i.Get("x")
	return files.Get(i.Name)
}

func Describe(i *Info) string { return i.Get("d") }

func ListOnly(n string) []string { return files.Get(n) }
