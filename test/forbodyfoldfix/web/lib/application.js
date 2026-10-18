'use strict'

const Stream = require('stream')

/**
 * Response helper: write the response body of `ctx` to the client.
 */
function respond (ctx) {
  if (ctx.respond === false) return
  if (!ctx.writable) return
  const res = ctx.res
  let body = ctx.body
  const code = ctx.status
  if (ctx.method === 'HEAD') {
    if (!res.headersSent && !ctx.response.has('Content-Length')) {
      ctx.length = Buffer.byteLength(JSON.stringify(body || ''))
    }
    return res.end()
  }
  if (body == null) {
    body = ctx.message || String(code)
    return res.end(body)
  }
  if (Buffer.isBuffer(body)) return res.end(body)
  if (typeof body === 'string') return res.end(body)
  if (body instanceof Stream) return body.pipe(res)
  body = JSON.stringify(body)
  res.end(body)
}

class Application {
  constructor (options) {
    this.middleware = []
    this.context = Object.create(null)
    this.response = Object.create(null)
    this.options = options || {}
  }

  use (fn) {
    this.middleware.push(fn)
    return this
  }

  handleRequest (ctx, fnMiddleware) {
    const handleResponse = () => respond(ctx)
    return fnMiddleware(ctx).then(handleResponse)
  }
}

module.exports = { Application, respond }
