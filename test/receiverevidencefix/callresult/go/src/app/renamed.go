package app

import (
	m "example.com/cr/src/model"
)

// UseRenamedImport: a renamed import `m.Get(..)` binds the package func, never the method Config.Get.
func UseRenamedImport(info *m.Info) {
	pkg, _ := m.Get("rpm")
	pkg.Package(info)
}
