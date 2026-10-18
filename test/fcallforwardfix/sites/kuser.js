const { FwdK } = require('./kmod');
function makeFwdK(o) { FwdK.call(o); return o; }
module.exports = { makeFwdK };
