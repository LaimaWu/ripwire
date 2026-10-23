#include "ops.h"
static void copy_command( int k ) { (void)k; }
static void copy_release( int k ) { (void)k; }
const struct mode copy_mode = { .command = copy_command, .release = copy_release };
void send_keys( struct mode_entry *wme ) { wme->mode->command( 1 ); }
void drop_mode( mode_alias *m ) { m->release( 2 ); }
static void on_cb( int x ) { (void)x; }
static const struct outer outer_one = { .in = { .cb = on_cb } };
void run_outer( const struct outer *o ) { o->in.cb( 1 ); }
