'use strict'
// [B1a] a module whose export is an OBJECT with a method named `call` — importing it proves a binding, not a function.
module.exports = { call (method, n) { return n } }
