#include "u.h"
int canon( int* path, long* bits, int* err )
{
    return *path + int( *bits ) + *err;
}
int canon( char* path, long* len, long* bits, int* err )
{
    return *path + int( *len + *bits ) + *err;
}
