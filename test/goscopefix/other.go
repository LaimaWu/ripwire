package scope

// Package is a free function that shares the method name: it keeps no scope.
func Package() error { return nil }

// Matcher has a Loop method; a function literal bound inside it is a local, not a member of Matcher.
type Matcher struct {
	limit int
}

func (m Matcher) Loop() int {
	var step = func() int { return m.limit }
	return step()
}

// Stack is generic: its methods scope to Stack, not Stack[T].
type Stack[T any] struct {
	items []T
}

func (s *Stack[T]) Push(v T) { s.items = append(s.items, v) }

func (Stack[T]) Size() int { return 0 }

// Build declares an interface inside a function body: its member scopes to the interface, not to Build.
func Build() int {
	type closer interface {
		Close() error
	}
	var c closer
	_ = c
	return Run(&Deb{})
}

func Run(d *Deb) int {
	_ = d.Package(nil)
	_ = Package()
	return len(d.ConventionalFileName())
}

// Terminal has a second Loop: a bare `Loop` names two methods, `Matcher::Loop` one.
type Terminal struct{}

func (t *Terminal) Loop() int { return Matcher{}.Loop() }
