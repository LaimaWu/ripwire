package cmd

import (
	"os"

	"example.com/dsf/pkgr"
)

func doPackage(target string, intoDir bool) error {
	info := pkgr.WithDefaults(&pkgr.Info{})
	pkg, err := pkgr.Get("deb")
	if err != nil {
		return err
	}
	if target == "" {
		target = pkg.ConventionalFileName(info)
	} else if intoDir {
		target = target + "/" + pkg.ConventionalFileName(info)
	}
	f, err := os.Create(target)
	if err != nil {
		return err
	}
	return pkg.Package(info, f)
}

func onlyResolved() *pkgr.Info {
	return pkgr.WithDefaults(&pkgr.Info{})
}
