'use strict'
function handleRequest (request, reply) {
  runRequestHooks(request)
  reply.send(request.body)
}
function runRequestHooks (request) {
  for (const hook of request.hooks || []) hook(request)
}
module.exports = { handleRequest, runRequestHooks }
