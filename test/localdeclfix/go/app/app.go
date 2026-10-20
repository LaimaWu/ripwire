package app

import "example.com/golocal/model"

// package-level vars and consts: these stay symbols
var pkgCfg = model.Make()

var (
	pkgGroupA = model.Group()
	pkgGroupB int
)

const pkgLimit = 8

const (
	pkgConstA = 1
	pkgConstB = 2
)

// shared name: a package-level var AND a function-local var named the same
var sharedName = 1

func UseWritten() int {
	var localCfg *model.Config = model.Make()
	var (
		localGroupA = model.Group()
		localGroupB int
	)
	localShort := model.Build()
	const localLimit = 3
	const (
		localConstA = 1
	)
	var sharedName = model.Count()
	_ = localCfg
	return localGroupA + localGroupB + localShort + localLimit + localConstA + sharedName
}

func UseClosure() func() int {
	return func() int {
		var closureVar = model.Count()
		return closureVar
	}
}

func UseNested(n int) int {
	for i := 0; i < n; i++ {
		if i > 1 {
			var nestedVar = model.Build()
			return nestedVar
		}
	}
	return 0
}

func UsePkg() int {
	return pkgGroupA + pkgGroupB + pkgLimit + pkgConstA + pkgConstB + sharedName + len(pkgCfg.Name) + pkgAfter
}

// a package-level var declared after the functions is still package scope
var pkgAfter = model.Count()
