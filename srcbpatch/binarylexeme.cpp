#include "stdafx.h"
#include "binarylexeme.h"
#include "fileprocessing.h"

namespace bpatch
{
using namespace std;

const span<const char>& AbstractBinaryLexeme::Replace(size_t at, size_t alength, unique_ptr<AbstractBinaryLexeme>& awith)
{
    auto cBegin = dataSequence_.cbegin();
    dataSequence_.erase(cBegin + at, cBegin + at + alength);
    cBegin = dataSequence_.cbegin();
    dataSequence_.insert(cBegin + at, awith->dataSequence_.cbegin(), awith->dataSequence_.cend());

    access_ = span<char>(dataSequence_.data(), dataSequence_.size());
    return access_;
}


unique_ptr<AbstractBinaryLexeme> AbstractBinaryLexeme::LexemeFromVector(vector<char>&& asrcVec)
{
    return unique_ptr<AbstractBinaryLexeme>(new AbstractBinaryLexeme(move(asrcVec)));
}

vector<char> CreateVectorWithData(const char* const pData, const size_t sz)
{
    return vector<char>(pData, pData + sz);
}

unique_ptr<AbstractBinaryLexeme> AbstractBinaryLexeme::LexemeFromStringView(const string_view asrc)
{
    return LexemeFromVector(move(CreateVectorWithData(asrc.data(), asrc.size())));
}


unique_ptr<AbstractBinaryLexeme> AbstractBinaryLexeme::LexemeFromSpan(const span<const char> asrc)
{
    return LexemeFromVector(move(CreateVectorWithData(asrc.data(), asrc.size())));
}


unique_ptr<AbstractBinaryLexeme> AbstractBinaryLexeme::LexemeFromLexeme(unique_ptr<AbstractBinaryLexeme>& source, const size_t length)
{
    auto& srcSpan = source->access();
    const size_t finalSize = srcSpan.size() < length ? srcSpan.size() : length;

    return LexemeFromSpan(span<const char>(srcSpan.data(), finalSize));
}



AbstractBinaryLexeme::AbstractBinaryLexeme(vector<char>&& asrcVec)
    : dataSequence_(move(asrcVec))
    , access_(dataSequence_.data(), dataSequence_.size())
{
}



};// namespace bpatch
