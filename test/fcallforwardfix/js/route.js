'use strict'
const handleRequest = require('./handle-request.js')
const { helperA } = require('./helpers.js')
const vendor = require('vendorpkg')
const rpc = require('./rpc.js')
const patch = require('./patchmod.js')
const { Klass, makeKlass } = require('./klass.js')

const notFn = { call () { return 0 } }

function routeHandler (request, reply) {
  runPreParsing(null, request, reply)
}

// [P1] `.call` on a relative require; [V1] `.bind` handed on as an argument is a value, not a call
function runPreParsing (err, request, reply) {
  if (err) { return }
  hookRunner(request, reply, handleRequest.bind(request.server))
  handleRequest.call(request.server, null, request, reply)
}

function hookRunner (request, reply, next) {
  return next
}

// [P2] `.apply` on a function this file declares BELOW the site (a declaration is hoisted)
function viaApply () {
  return localDecl.apply(this, arguments)
}

function localDecl () { return 7 }

// [P3] `.call` on a destructured relative require
function helperUser () {
  return helperA.call(null, 1)
}

// [N1] a parameter of the site's function shadows the namesake in other.js
function nearParam (cb) {
  return cb.call(this)
}

// [N2] a method-valued member, never the bare name
function nearMember (obj) {
  return obj.method.call(this)
}

// [N3] a file-scope NON-function binding
function nearNonFunc () {
  return notFn.call(this)
}

// [N4] an import from outside the tree
function nearExternal () {
  return vendor.call(this)
}

// [N5] a loop variable
function nearLoop (hooks) {
  for (const hook of hooks) { hook.call(this) }
}

// [N6] a global object and a prototype method
function nearGlobal (x) {
  Reflect.apply(target, this, [])
  return Object.prototype.toString.call(x)
}

// [V2] only ever bound, never called: a value reference
function onlyBound () { return 8 }
function binder (emitter) {
  emitter.on('x', onlyBound.bind(this))
}

// [H] several candidate definitions: the forwarded call is decided as the bare call is
function outer1 () { function dup () { return 1 } return dup() }
function outer2 () { function dup () { return 2 } return dup() }
function dupForwarded () { return dup.call(this) }
function dupBare () { return dup() }

// [B1a] an imported module OBJECT whose method is named call / apply: no forwarded call (the import proves no function)
function nearObjImport (doc) {
  rpc.call('x', 1)
  return patch.apply(doc)
}

// [B1c] an imported class: no forwarded call; [P] the function imported beside it: an edge
function nearClassImport () {
  Klass.call(this)
  return makeKlass.call(this)
}

// [M2] a file-scope variable rebinds a declared function's name: no forwarded call
function fv () { return 15 }
var fv = 3
function nearRebound () {
  return fv.call(this)
}

// [q10] a function declared only inside a SIBLING scope: the bare name is unbound here, so no forwarded call
function hostA () { function nestedOnly () { return 16 } return nestedOnly() }
function nearNested () {
  return nestedOnly.call(this)
}

module.exports = { routeHandler, viaApply, helperUser, nearParam, nearMember, nearNonFunc, nearExternal, nearLoop, nearGlobal, binder, outer1, outer2, dupForwarded, dupBare, nearObjImport, nearClassImport, nearRebound, hostA, nearNested }
