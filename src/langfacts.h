#pragma once
// langfacts.h — the per-language FACTS table (the "6c" step of the language-specific layer): one row per `Lang`, one column
// per fact that shared code reads about a language, instead of a `== Lang::X` compare at the point of use.
//
// WHY A TABLE. A shared file that says `if( r.lang == Lang::Ruby )` hides the reason (what does Ruby have that the others do not?)
// and hides the next language that has it. A column is named for its REASON, reads down the table beside every other language,
// and a new language is one more row instead of an edit at every site. CONTRIBUTING's rule applies: a new `== Lang::X` in a shared
// file is a red flag, and test/langidentitycheck.sh's ratchet counts them.
//
// THE RULES.
//   * A column is a fact about a language, decided per language: a bool, an enum or a small list. Never a function.
//   * A column is named for its reason (`overloadSignatures`), never for a language (`isTypeScript`) or a family (`isCFamily`).
//   * An OPTIONAL column defaults to the conservative answer: `false` = "the shared path", so a new language that says nothing
//     gets the behaviour every other language without the fact gets.
//   * A REQUIRED column is typed `Must<T>`: it has no default constructor, so a row that leaves it out of its designated
//     initializer does not compile. Today only the row's own key (`lang`) is required; a column every new language must decide
//     (and cannot sensibly default) is declared `Must<...>` the day it is added.
//   * The table is `std::array<LangFacts, kLangCount>`: a missing row is a compile error (the unwritten element has no `lang`),
//     and `firstMisorderedRow` proves each row sits at its own `Lang` index, so langFacts(l) is one load.
//   * Reading a language past the enum (a corrupt cache byte) answers the all-default row, like the other enum-indexed tables.
//
// WHAT IS NOT HERE. Hooks (an algorithm that differs over the tree or the graph) and family-owned sparse rows live beside their
// shared handler (reports/lang-specific-layer.md §2); this header holds only the values. It includes nothing but model.h, so both
// translation units (ingest.cpp and main.cpp) can read it.

#include "model.h"

#include <algorithm>
#include <array>
#include <cstddef>

namespace rw
{

/// A required column of a LangFacts row: no default constructor, so a designated initializer that omits it fails to compile.
template<class T>
struct Must
{
    T value;
    constexpr Must( T v ) noexcept : value( v ) {}   // implicit on purpose: `.lang = Lang::Cpp` spells the row's key
    constexpr operator T() const noexcept { return value; }
};

/// What shared code asks about a language. Every optional column is false for a language that says nothing.
struct LangFacts
{
    Must<Lang> lang;   // the row's own key — the order check below proves row i is Lang(i)

    /// TypeScript: a bodyless function, or a bodyless method that is not an interface / abstract / ambient contract, is an OVERLOAD
    /// SIGNATURE: the language requires the implementation to follow it in the same declaration list, so it joins that implementation
    /// as one definition (defattrib.h). In Java, Go, Rust, Swift or C# a bodyless member is a contract and a definition of its own.
    bool overloadSignatures = false;

    /// Ruby: an instance receiver resolves through the language's own class-object lookup (the singleton / instance sides of a
    /// class, a typed or closed receiver, `self` through included modules) before the shared receiver rules (graph.h).
    bool classObjectLookup = false;

    /// Ruby: a method's ancestors include mixed-in modules, so a base-class walk climbs the mixin-aware chain rather than the
    /// declared inheritance chain (graph.h).
    bool mixinAncestry = false;

    /// Ruby: an element access `recv[ k ]` whose receiver the shape rules cannot classify (a call result, a richer expression) is
    /// still recorded as a site on that receiver, because the typing walk may type it later (`Report.new[ :x ]`). Python and the
    /// rest record nothing for such a receiver (ingest_binds.h subscriptOperatorRef).
    bool keepsUnshapedSubscript = false;

    /// Ruby: a constant receiver is a path as written (`Outer::Cache`), kept on the site so the resolver scopes it; a name has no
    /// other spelling in the languages without constant paths (ingest_binds.h subscriptOperatorRef).
    bool constantPathReceiver = false;

    /// Elixir: `quote` bodies are inert syntax (an AST being built, not code that runs), so a call candidate inside one is a
    /// declined fact, never an edge (ingest_sidecap.h; the predicate itself lives in ingest_elixir.h).
    bool hasInertQuoteRegions = false;

    /// C++: defect-shape's whole-tree input is the macro-name set, and a changed C++ file (a .h reads as C++) can only be
    /// judged against the base's whole set. A changed C or Objective-C file does not need it, by today's behaviour
    /// (verbs_quality.h; pinned by test/qddegradecheck.sh (E3)).
    bool defectShapeNeedsMacroSet = false;

    /// Rust: a trait's bodyless method signature is no definition the index keeps (only a bodied default is), so a trait's contract
    /// cannot be read off its members. The dispatch tier (ifacedispatch.h) takes the implementors' methods of the called name without
    /// it, and leaves a standard-member name (`clone`, `iter`: graph.h StdMemberGate) to the name tiers as before.
    bool contractSignaturesUnindexed = false;

    /// Rust: a type's methods for a trait sit in that trait's own `impl T for X` block, apart from its inherent `impl X` methods,
    /// and a call through `dyn T` / `impl T` / a `T` bound reaches only the former and T's defaults (ingest_relations.h records each
    /// trait impl block as an ImplBlock binding; ifacedispatch.h keeps the candidates inside one).
    bool traitImplBlocks = false;

    /// Go, TypeScript, JavaScript: a type conforms to an interface (TS: to any object type) by its members alone, with no clause
    /// naming it — an object literal, a structurally matching class — so an implements list is never the whole set, and one
    /// candidate is never the only implementor (ifacedispatch.h: a list of two or more is still declined, as a floor).
    bool structuralTyping = false;

    /// Java: a lambda or a method reference implements any interface with exactly one abstract method, with no class the index
    /// keeps, so such an interface's implementors list is open (ifacedispatch.h). Kotlin opts in per interface (`fun interface`,
    /// recorded at ingest as an anonymous implementor).
    bool samLambdas = false;

    /// Java, Kotlin: an expression can implement an interface or extend a class anonymously (`new T() { … }`, `object : T { … }`;
    /// Kotlin's `fun interface` lets a lambda do it), recorded at ingest as an ImplBlock binding with no implementor name, which
    /// opens that type's implementors list (ingest_binds.h anonymousImplementorOf).
    bool anonymousImplementors = false;

    /// Markdown: a link targets a section, so shared graph code treats this language's sections as link targets (graph.h).
    bool linkSections = false;

    /// C, C++, Objective-C: a function span the parser reports DIRECTLY inside a function's span is read as a misread (htop's
    /// `else if (…) {` inside a macro-heavy body read as a function named `if`), never a closure (src/closures.h; this fact
    /// decides only the closure relation, no symbol or edge). A lambda or a block is anonymous, and a C++ local class's
    /// methods sit inside the class's scope, not directly in the function, so neither is a closure either way.
    bool noNestedNamedFunctions = false;
};

/// The rows, one per Lang in enum order. A language says only the facts it has.
inline constexpr std::array<LangFacts, kLangCount> kLangFacts = { {
    { .lang = Lang::Cpp, .defectShapeNeedsMacroSet = true, .noNestedNamedFunctions = true },
    { .lang = Lang::Python },
    { .lang = Lang::TypeScript, .overloadSignatures = true, .structuralTyping = true },
    { .lang = Lang::Go, .structuralTyping = true },
    { .lang = Lang::Rust, .contractSignaturesUnindexed = true, .traitImplBlocks = true },
    { .lang = Lang::Swift },
    { .lang = Lang::ObjC, .noNestedNamedFunctions = true },
    { .lang = Lang::Markdown, .linkSections = true },
    { .lang = Lang::JavaScript, .structuralTyping = true },
    { .lang = Lang::Bash },
    { .lang = Lang::Java, .samLambdas = true, .anonymousImplementors = true },
    { .lang = Lang::Ruby, .classObjectLookup = true, .mixinAncestry = true, .keepsUnshapedSubscript = true, .constantPathReceiver = true },
    { .lang = Lang::Unknown },
    { .lang = Lang::Json },
    { .lang = Lang::CSharp },
    { .lang = Lang::C, .noNestedNamedFunctions = true },
    { .lang = Lang::Toml },
    { .lang = Lang::Yaml },
    { .lang = Lang::Php },
    { .lang = Lang::Lua },
    { .lang = Lang::Elixir, .hasInertQuoteRegions = true },
    { .lang = Lang::Dart },
    { .lang = Lang::Kotlin, .anonymousImplementors = true },
    { .lang = Lang::GDScript },
    { .lang = Lang::Clojure },
} };

/// The index of the first row that is not at its own Lang index, or the table size when every row is.
template<std::size_t N>
constexpr std::size_t firstMisorderedRow( const std::array<LangFacts, N>& table ) noexcept
{
    const auto misplaced = std::ranges::find_if( table, [ expected = std::size_t( 0 ) ]( const LangFacts& row ) mutable
                                                 { return static_cast<std::size_t>( static_cast<Lang>( row.lang ) ) != expected++; } );
    return static_cast<std::size_t>( misplaced - table.begin() );
}
static_assert( firstMisorderedRow( kLangFacts ) == kLangCount, "kLangFacts: one row per Lang, in enum order — the compiler's note 'evaluates to A == B' gives the first row out of place as A (B = kLangCount)" );

/// The row of `l`; a value past the enum (a corrupt cache byte) reads the all-default row.
inline constexpr LangFacts langFacts( Lang l ) noexcept
{
    return enumTableAt( kLangFacts, l, LangFacts { .lang = Lang::Unknown } );
}

}   // namespace rw
