/** Content negotiation: pick a response type from the Accept header. */
export function negotiate(req: Request): string {
  const accept = req.headers.get('accept') || '*/*'
  if (accept.includes('application/json')) return 'json'
  if (accept.includes('text/html')) return 'html'
  return 'text' // negotiate fallback when nothing in Accept matches
}
