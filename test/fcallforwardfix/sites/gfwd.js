const { notThere } = require('./kmod');
function gCaller() { return notThere.call(null); }
module.exports = { gCaller };
