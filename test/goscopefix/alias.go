package scope

// Note's method has a comment inside its receiver list: the receiver is still the scope (Body sc="Note").
type Note struct{}

func ( /* the note */ n *Note) Body() string { return "" }

// Sink is an alias to an interface literal: its member scopes to the alias name (Flush sc="Sink").
type Sink = interface {
	Flush() error
}
