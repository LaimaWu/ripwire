#include "eval.h"

// keep the hedge: the container's recorded element type is EvalString
bool ruleSubscript( const std::string& name ) { auto rule = std::unique_ptr<Rule>( new Rule() ); return rule->bindings_[ name ].empty(); }
bool refDirect( Rule& r ) { return r.direct_[ "a" ].empty(); }
bool refVector( Rule& r ) { return r.vec_[ 0 ].empty(); }
bool localMap() { std::map<std::string, EvalString> m; return m[ "a" ].empty(); }
bool constMapAt( const std::map<std::string, EvalString>& m ) { return m.at( "a" ).empty(); }
bool unorderedMap() { std::unordered_map<std::string, EvalString> m; return m[ "a" ].empty(); }
bool vectorBack( std::vector<EvalString>& v ) { return v.back().empty(); }
bool vectorFront( std::vector<EvalString>& v ) { return v.front().empty(); }
bool typedefList( EvalList& v ) { return v[ 0 ].empty(); }
bool typedefOfTypedef( EvalList2& v ) { return v.at( 0 ).empty(); }
bool globalTable() { return g_table[ 1 ].empty(); }

// near-misses: declined
bool mapOfString( Rule& r ) { return r.strs_[ "a" ].empty(); }
bool vectorOfString( std::vector<std::string>& v ) { return v[ 0 ].empty(); }
bool mapOfVector() { std::map<std::string, std::vector<EvalString>> m; return m[ "a" ].empty(); }
bool elementNoMember( Rule& r ) { return r.others_[ 0 ].empty(); }
bool containerItself( Rule& r ) { return r.vec_.empty(); }
bool inTreeTemplate( ns::Grid<EvalString>& g ) { return g[ 0 ].empty(); }

// stated floors: declined
bool twoLevels() { std::map<std::string, std::vector<EvalString>> m; return m[ "a" ][ 0 ].empty(); }
bool iteratorSecond( std::map<std::string, EvalString>& m ) { auto it = m.find( "a" ); return it->second.empty(); }
bool rangeForAuto( std::vector<EvalString>& v ) { for( auto& e : v ) { return e.empty(); } return false; }
