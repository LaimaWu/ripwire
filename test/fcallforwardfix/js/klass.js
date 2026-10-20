'use strict'
// [B1c] an imported CLASS (Klass) beside an imported function (makeKlass, the [P] twin).
class Klass { constructor () { this.n = 1 } }
function makeKlass () { return new Klass() }
module.exports = { Klass, makeKlass }
