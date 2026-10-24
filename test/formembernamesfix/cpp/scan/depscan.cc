// Out-of-line definitions of the dirty scan.
#include "depscan.h"

DependencyScan::DependencyScan( int depth ) : depth_( depth ) {}

bool DependencyScan::RecomputeDirty( Node* node, std::string* err )
{
    (void)node;
    (void)err;
    return depth_ > 0;
}

bool DependencyScan::RecomputeOutputDirty( Node* output, long mostRecentInput )
{
    (void)output;
    return mostRecentInput > depth_;
}

void DependencyScan::LoadDepfile( const std::string& path )
{
    depth_ += int( path.size() );
}

bool ScanAllDirty( DependencyScan& scan, Node* root )
{
    std::string err;
    return scan.RecomputeDirty( root, &err );
}
