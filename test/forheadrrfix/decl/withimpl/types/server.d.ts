/** Handle an incoming request: route the request, run the request hooks, and send the reply for the request. */
export declare function handleRequest(request: unknown, reply: unknown): void;
/** The request hooks that run before the request handler sends the reply. */
export declare function runRequestHooks(request: unknown): void;
