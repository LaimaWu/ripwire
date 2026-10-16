package rpm

import (
	"io"

	"example.com/dsf/pkgr"
)

type RPM struct{}

func (RPM) ConventionalFileName(info *pkgr.Info) string {
	return info.Target + ".rpm"
}

func (RPM) Package(info *pkgr.Info, w io.Writer) error {
	return nil
}
