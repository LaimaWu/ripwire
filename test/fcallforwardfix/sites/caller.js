function fwdHost() { function fwdNested(x) { return x; } return fwdNested; }
function fwdCaller() {
  fwdNested(1);
  return fwdNested.call(this, 1, 2, 3);
}
module.exports = { fwdHost, fwdCaller };
