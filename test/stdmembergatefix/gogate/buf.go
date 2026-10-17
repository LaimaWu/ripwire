package gogate

// Go ships an EMPTY row (graph.h kStdMemberNames): Len / Write / String are methods in-tree types implement on purpose.
type Buf struct{ n int }

func (b *Buf) Len() int { return b.n }

func lenOfResult(f func() *Buf) int { return f().Len() }
