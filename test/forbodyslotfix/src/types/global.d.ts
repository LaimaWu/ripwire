/** Ambient declarations for the server runtime. */
/** negotiate: pick the content type for a response from the request's Accept header (json, html or text). */
export function negotiate(req: Request): string
export declare function handshake(socket: WebSocket, protocol: string): Promise<boolean>
