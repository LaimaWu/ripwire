// The `debug` package idiom: extend() derives a sub-NAMESPACE logger, not a class.
const createDebug = require('debug');
const _ = require('lodash');
const log = createDebug('app');
const error = log.extend('error');
const verbose = log.extend('verbose');
const merged = _.extend('merged', { a: 1 });
function run() { error('boom'); verbose(merged); }
module.exports = { run };
