'use strict'
// Namesakes: a function of each near-miss name, so a forwarded near-miss would show as a FALSE edge here.
function cb () { return 1 }
function hook () { return 2 }
function notFn () { return 3 }
function method () { return 4 }
function vendor () { return 5 }
function target () { return 6 }
function rpc () { return 10 }
function patch () { return 11 }
function notAFn () { return 12 }
function nestedOnly () { return 13 }
function fv () { return 14 }
module.exports = { cb, hook, notFn, method, vendor, target, rpc, patch, notAFn, nestedOnly, fv }
