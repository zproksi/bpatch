#pragma once
#include <list>
#include <memory>
#include <unordered_map>

#include "streamreplacer.h"

namespace bpatch
{
/// <summary>
///  class Holds binary lexemes and provides access to them by name
/// </summary>
class Dictionary final
{
public:
    /// <summary>
    ///   access to Binary lexeme by name
    /// </summary>
    /// <param name="aname"> name of the lexeme from dictionary file</param>
    /// <returns>lexeme from dictionary file, or nullptr</returns>
    std::unique_ptr<AbstractBinaryLexeme>& Lexeme(const std::string_view aname) const;


    /// <summary>
    ///   return pair of Binary lexemes by name
    /// </summary>
    /// <param name="anameSrc">name of the lexeme from dictionary file</param>
    /// <param name="anameTrg">name of the lexeme from dictionary file</param>
    /// <returns>lexeme from dictionary file, or nullptr</returns>
    AbstractLexemesPair LexemesPair(const std::string_view anameSrc, const std::string_view anameTrg) const;


    /// <summary>
    ///   moves binary lexeme into dictionary
    /// </summary>
    /// <param name="aname">set the name for lexeme</param>
    /// <param name="alexeme">lexeme to store inside</param>
    /// <returns>true - if this lexeme is just added;
    ///  false - if lexeme already was in the dictionary: new lexeme is lost</returns>
    bool AddBinaryLexeme(const std::string_view aname, std::unique_ptr<AbstractBinaryLexeme>&& alexeme);

protected:
    using LEXEMES_HOLDER = std::list<std::unique_ptr<AbstractBinaryLexeme>>;
    /// <summary>
    ///   lexemes wich are under our responsibility
    /// </summary>
    LEXEMES_HOLDER lexemes_;

    /// <summary>
    ///  for fast search of lexeme
    /// </summary>
    std::unordered_map<std::string_view, LEXEMES_HOLDER::iterator> dict;

    /// <summary>
    ///  Default lexeme returned when result lexeme is not found
    /// </summary>
    std::unique_ptr<AbstractBinaryLexeme> empty_result;
}; // class Dictionary




};// namespace bpatch
