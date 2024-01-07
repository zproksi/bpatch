#include "stdafx.h"
#include "binarylexeme.h"
#include "dictionary.h"


namespace bpatch
{

std::unique_ptr<AbstractBinaryLexeme>& Dictionary::Lexeme(const std::string_view& aname) const
{
    if (auto it = dict.find(aname); it != dict.cend())
    {
        return *it->second;
    }
    return const_cast<std::unique_ptr<AbstractBinaryLexeme>&>(empty_result);
}


AbstractLexemesPair Dictionary::LexemesPair(const std::string_view& anameSrc, const std::string_view& anameTrg) const
{
    return {Lexeme(anameSrc), Lexeme(anameTrg)};
}


bool Dictionary::AddBinaryLexeme(const std::string_view& aname, std::unique_ptr<AbstractBinaryLexeme>&& alexeme)
{
    auto it = lexemes_.emplace(lexemes_.cend(), std::move(alexeme));

    auto [it_ignore, added] = dict.insert(
        std::pair<std::string_view, LEXEMES_HOLDER::iterator>(
        aname, it));
    return added;
}

};