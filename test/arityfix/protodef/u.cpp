#include "u.h"
int canon2( int* path, long* bits, int* err )
{
    return *path + int( *bits ) + *err;
}
int canon2( char* path, long* len, long* bits, int* err )
{
    return *path + int( *len + *bits ) + ( err ? *err : 0 );
}
