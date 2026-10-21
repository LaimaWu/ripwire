// The dependency graph's dirty scan: decides which outputs must be rebuilt.
#pragma once
#include <string>
#include <vector>

struct Node;

struct DependencyScan
{
    explicit DependencyScan( int depth );

    bool RecomputeDirty( Node* node, std::string* err );
    bool RecomputeOutputDirty( Node* output, long mostRecentInput );
    void LoadDepfile( const std::string& path );
    void Reset();
    int  Width() const;
    int  depth_ = 0;
};
