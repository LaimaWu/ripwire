static int twice( int n )
{
    return n * 2;
}

template <class Engine>
int drive( Engine& engine, int n )
{
    return engine.run( twice( n ) );
}
