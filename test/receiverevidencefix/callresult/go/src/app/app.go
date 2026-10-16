package app

import (
	"os"

	"example.com/cr/src/model"
)

// UseValue: a value result through named results (nfpm's `config, err := nfpm.ParseFile(..)`).
func UseValue(path string) {
	config, err := model.ParseFile(path)
	_ = err
	config.Get("deb")
}

// UsePtr: `(*T, error)`.
func UsePtr(path string) {
	c, _ := model.Load(path)
	c.Get("rpm")
}

// UseOne: a single `*T` result.
func UseOne() {
	c := model.Make()
	c.Get("apk")
}

// UsePair: the second result binds the second name.
func UsePair() {
	_, o := model.Pair()
	o.Get("ipk")
}

// UseIface: an interface result types the binding as the interface.
func UseIface(info *model.Info) {
	pkg, _ := model.Lookup("deb")
	pkg.Package(info)
}

// UseMethod: the callee is a method on a typed receiver.
func UseMethod(b *model.Builder) {
	c := b.Build()
	c.Get("arch")
}

// UseReassigned: Go is statically typed; `c = …` keeps c's type.
func UseReassigned() {
	c := model.Make()
	c = model.Make()
	c.Get("deb")
}

// NearExternal: an outside callee types nothing.
func NearExternal() {
	f, _ := os.Open("x")
	f.Close()
}

// NearTwoDefs: the callee binds two definitions (an untyped receiver).
func NearTwoDefs(factory interface{ Build() }) {
	x := factory.Build()
	x.Get("deb")
}

// NearAlias: a Go alias emits no alias record today.
func NearAlias() {
	a := model.MakeAlias()
	a.Get("deb")
}

// NearWrapper: a wrapper outside the list.
func NearWrapper() {
	r := model.Wrap()
	r.Get("deb")
}

// NearGeneric: a type-parameter result.
func NearGeneric() {
	g := model.Generic[model.Config]()
	g.Get("deb")
}

// NearShadow: an inner `c` hides the typed outer one.
func NearShadow() {
	c := model.Make()
	_ = c
	{
		c, _ := os.Open("y")
		c.Close()
	}
}

// UseWritten (twin): the same call with the type written — the record the receiver rules already read.
func UseWritten() {
	var c *model.Config = model.Make()
	c.Get("deb")
}

// UseQualifiedFn: `model.Get` reaches the package-level func only, never the method (nfpm's `pkg, err := nfpm.Get(..)`).
func UseQualifiedFn(info *model.Info) {
	pkg, _ := model.Get("deb")
	pkg.Package(info)
}

// NearQualified: an in-tree callee returning an OUTSIDE qualified type (*os.File) whose final segment names the
// in-tree model.File: the qualifier resolves to no in-tree package, so nothing is recorded.
func NearQualified() {
	f, _ := model.OpenOS()
	f.Close()
}

// NearOSParam: the written twin of the same outside type (a parameter): f.Close is os.File's.
func NearOSParam(f *os.File) {
	f.Close()
}

// UseGrouped / UseGroupedLast: grouped named results `(a, b *Config, o *Other)` count one position per NAME.
func UseGrouped() {
	_, y, _ := model.Triple()
	y.Get("deb")
}

func UseGroupedLast() {
	_, _, z := model.Triple()
	z.Get("rpm")
}

// NearTypeParamNamed: a type parameter named after the in-tree Config, instantiated with Other.
func NearTypeParamNamed() {
	g := model.Generic2[model.Other]()
	g.Get("deb")
}

// NearShadowPkg: a local spelled like the import is not the package — `model.Get` here is a method call on the local.
func NearShadowPkg() {
	model := model.Make()
	model.Get("x")
}
