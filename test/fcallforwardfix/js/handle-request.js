'use strict'
// The fastify shape: a module whose export is ONE function, reached from route.js only through `.call` / `.bind`.
function handleRequest (err, request, reply) {
  if (err) { return }
  preHandlerInner(err, request, reply)
}

function preHandlerInner (err, request, reply) {
  reply.done(request)
}

module.exports = handleRequest
