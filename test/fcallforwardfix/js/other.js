'use strict'
// Namesakes: a function of each near-miss name, so a forwarded near-miss would show as a FALSE edge here.
function cb () { return 1 }
function hook () { return 2 }
function notFn () { return 3 }
function method () { return 4 }
function vendor () { return 5 }
function target () { return 6 }
module.exports = { cb, hook, notFn, method, vendor, target }
