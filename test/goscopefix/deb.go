package scope

import "io"

// Deb implements Packager on a pointer receiver.
type Deb struct {
	Arch string
}

func (d *Deb) Package(w io.Writer) error {
	_, err := w.Write([]byte(d.ConventionalFileName()))
	return err
}

func (d *Deb) ConventionalFileName() string { return "pkg_" + d.Arch + ".deb" }
