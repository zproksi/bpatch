#pragma once
#include <memory>
#include <utility>
#include <vector>

namespace bpatch
{
/// <summary>
///   forward declarations for factory methods
/// </summary>
class Writer;
class AbstractBinaryLexeme;


/// <summary>
///   source is first           (will be replaced)
///   target is second in pair  (with this lexem)
/// </summary>
typedef std::pair<std::unique_ptr<AbstractBinaryLexeme>&,
    std::unique_ptr<AbstractBinaryLexeme>&> AbstractLexemesPair;
/// <summary>
///   one replaces possible among provided pairs
/// </summary>
typedef std::vector<AbstractLexemesPair> StreamReplacerChoice;
//--------------------------------------------------


/// <summary>
///    stream for bytes to replace
/// </summary>
struct StreamReplacer
{
    /// <summary>
    ///   callback from StreamReplacer
    /// </summary>
    /// <param name="toProcess">char - next in sequence</param>
    /// <param name="aEod">this is sign that no more data,
    ///   current char 'toProcess' is not valid if aEod is true</param>
    virtual void DoReplacements(const char toProcess, const bool aEod) const = 0;


    /// <summary>
    ///   callback from StreamReplacer
    /// </summary>
    /// <param name="toProcess">replacer to call next</param>
    virtual void SetNextReplacer(StreamReplacer* const pNext) = 0;


    virtual ~StreamReplacer() = default;


// how to create Stream Replacers
public:

/// <summary>
///    creates a StreamReplacer for writing the lexemes to file
/// </summary>
/// <param name="pWriter">writer interface</param>
/// <returns>unique_ptr with object which should be last in chain of processing</returns>
static std::unique_ptr<StreamReplacer> ReplacerLastInChain(Writer* const pWriter);


/// <summary>
///  creates replacer for choose specific lexeme among others to replace
/// </summary>
/// <param name="choice">set of pairs of src & trg lexemes - one of which
///   can be processed. The one that was found first.
///  if it has only one pair than it's ok
///  </param>
/// <returns>Replacer for building replacement chain</returns>
static std::unique_ptr<StreamReplacer> CreateReplacer(StreamReplacerChoice& choice);

};

};// namespace bpatch
