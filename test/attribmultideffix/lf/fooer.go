package lf

// lang-facts pins: a Go interface method is a contract, a definition of its own.
type Fooer interface{ Foo() }
type T struct{}

func (T) Foo() {}
func Use(f Fooer) { f.Foo() }
