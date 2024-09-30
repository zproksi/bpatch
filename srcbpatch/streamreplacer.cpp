#include "stdafx.h"
#include "binarylexeme.h"
#include "fileprocessing.h"
#include "streamreplacer.h"

namespace bpatch
{
using namespace std;

//--------------------------------------------------
/// <summary>
///   Replacer to last in chain. All incoming data should be trasferred to a Writer interface
/// </summary>
class WriterReplacer final : public StreamReplacer
{
public:
    WriterReplacer(Writer* const pWriter): pWriter_(pWriter) {};

    virtual void DoReplacements(const char toProcess, const bool aEod) const override;
    virtual void SetNextReplacer(std::unique_ptr<StreamReplacer>&& pNext) override;
protected:
    Writer* const pWriter_;
};


void WriterReplacer::DoReplacements(const char toProcess, const bool aEod) const
{
    pWriter_->WriteCharacter(toProcess, aEod);
}


void WriterReplacer::SetNextReplacer(std::unique_ptr<StreamReplacer>&&)
{
    throw logic_error("Writer Replacer should be unchangeable. Contact with maintainer.");
}


unique_ptr<StreamReplacer> StreamReplacer::ReplacerLastInChain(Writer* const pWriter)
{
    return unique_ptr<StreamReplacer>(new WriterReplacer(pWriter));
}

//--------------------------------------------------

/// <summary>
///     common part for set next replacer
/// </summary>
class ReplacerWithNext: public StreamReplacer
{
protected:
    /// to pass processing further
    std::unique_ptr<StreamReplacer> pNext_;

public:
    void SetNextReplacer(std::unique_ptr<StreamReplacer>&& pNext) override
    {
        std::swap(pNext_, pNext);
    }

};

/// <summary>
///     a class with some common methods for all replacers
/// </summary>
class BaseReplacer: public ReplacerWithNext
{
protected:
    /// <summary>
    ///   Sends target to next replacers, and resets partial match index to zero
    /// </summary>
    /// <param name="target">the array we need to send</param>
    void SendFurther(const span<const char>& target) const
    {
        for (const char c : target)
        {
            pNext_->DoReplacements(c, false);
        }
    }
};

/// <summary>
///     a class with some common methods for all replacers including cache
/// </summary>
class BaseReplacerWithCache: public BaseReplacer
{
protected:
    /// <summary>
    ///   Clean srcMatchedLength bytes of cache from the beginning
    /// </summary>
    /// <param name="srcMatchedLength">number of bytes we have to clear</param>
    void CleanTheCache(size_t srcMatchedLength) const
    {
        shift_left(cachedData_.data(),
            cachedData_.data() + cachedAmount_,
            static_cast<std::iterator_traits<decltype(cachedData_.data())>::difference_type>(srcMatchedLength));
        cachedAmount_ -= srcMatchedLength;
    }

protected:
    mutable size_t cachedAmount_ = 0; // we cached this amount of data

    // this is used to hold temporary data while the logic is
    // looking for the new beginning of the cached value
    mutable vector<char> cachedData_;
};


//--------------------------------------------------
struct ReplacerPairHolder
{
    ReplacerPairHolder(unique_ptr<AbstractBinaryLexeme>& src,  // what to replace
        unique_ptr<AbstractBinaryLexeme>& trg)                 // to what
        : src_(src)
        , trg_(trg)
    {}

    unique_ptr<AbstractBinaryLexeme>& src_; // what to replace
    unique_ptr<AbstractBinaryLexeme>& trg_; // with what
};


//Description????
/// The class finds the lexeme src_ and replaces it to trg_, the src_ and trg_ are non empty strings
class UsualReplacer final : public BaseReplacerWithCache
{
public:
    UsualReplacer(unique_ptr<AbstractBinaryLexeme>& src,  // what to replace
        unique_ptr<AbstractBinaryLexeme>& trg)  // with what
        : src_(src->access())
        , trg_(trg->access())
    {
        cachedData_.resize(src_.size());
    }

    void DoReplacements(const char toProcess, const bool aEod) const override;

protected:
    /// <summary>
    ///   We got the 'end' character so there are no match -> we should pass further all the cache
    /// </summary>
    /// <param name="toProcess">character received along with end of data sign</param>
    void DoReplacementsAtTheEndOfTheData(const char toProcess) const
    {
        SendFurther(std::span<char> (cachedData_.data(), cachedAmount_));
        CleanTheCache(cachedAmount_);
        pNext_->DoReplacements(toProcess, true);
    }
    const span<const char>& src_; // what to replace
    const span<const char>& trg_; // with what
};


void UsualReplacer::DoReplacements(const char toProcess, const bool aEod) const
{
    if (nullptr == pNext_)
    {
        throw logic_error("Replacement chain has been broken. Communicate with maintainer");
    }

    if (aEod) [[unlikely]]
    {
        DoReplacementsAtTheEndOfTheData(toProcess);
        return;
    }

    cachedData_[cachedAmount_++] = toProcess;
    // our cachedData_ should contain only prefix of src_, otherwise -> clean the cache from the beginning
    while (cachedAmount_ > 0 && memcmp(cachedData_.data(), src_.data(), cachedAmount_) != 0)
    {
        SendFurther(std::span<char> (cachedData_.data(), 1));
        CleanTheCache(1);
    }

    if (cachedAmount_ == src_.size())
    {
        SendFurther(trg_);
        CleanTheCache(cachedAmount_);
    }
}


/// <summary>
///  creates replacer
/// </summary>
/// <param name="src">what we are going to replace</param>
/// <param name="trg">this is the result of the replacement</param>
/// <returns>Replacer for building replacement chain</returns>
static unique_ptr<StreamReplacer> CreateSimpleReplacer(
    unique_ptr<AbstractBinaryLexeme>& src,  // what to replace
    unique_ptr<AbstractBinaryLexeme>& trg)  // with what
{
    return unique_ptr<StreamReplacer>(new UsualReplacer(src, trg));
}


//--------------------------------------------------
///
///      |--SRC 1  TRG 1  |
///  O - |-- ...          | - o
///      |--SRC N  TRG N  |
///
/// Description????
class ChoiceReplacer final : public BaseReplacerWithCache
{
    typedef struct
    {
        span<const char> src_;
        span<const char> trg_;
    }ChoiceReplacerPair;

public:
    /// <summary>
    ///   creating ChoiceReplacer from provided pairs
    /// </summary>
    /// <param name="choice">vector os source & target pairs</param>
    ChoiceReplacer(StreamReplacerChoice& choice)
    {
        size_t bufferSize = 0; // to allocate buffer
        const size_t sz = choice.size();
        rpairs_.resize(sz);
        for (size_t i = 0; i < sz; ++i)
        {
            auto& vPair = choice[i]; // copy from
            auto& rpair = rpairs_[i];// copy to

            rpair.src_ = vPair.first->access();
            const size_t sourceSize = rpair.src_.size();
            if (bufferSize < sourceSize)
            {
                bufferSize = sourceSize; // calculate necessary buffer size
            }

            rpair.trg_ = vPair.second->access();
        }

        cachedData_.resize(bufferSize);
    }

    void DoReplacements(const char toProcess, const bool aEod) const override;

protected:
    /// <summary>
    ///    check for partial or full match of the data from cachedData_
    ///       with any of lexemes sequentially
    ///       and provides type of the match and index if found
    /// </summary>
    /// <param name="indexFrom"> search in pairs from this index</param>
    /// <param name="fullOnly"> what type of match we want to find : only full or partial also </param>
    /// <returns>bool: partial, bool: full, size_t: index </returns>
    tuple<bool, bool, size_t> FindMatch(const size_t indexFrom, const bool fullOnly) const
    {
        for (size_t i = indexFrom; i < rpairs_.size(); ++i)
        {
            const auto& srcSpan = rpairs_[i].src_;
            const size_t cmpLength = (srcSpan.size() > cachedAmount_) ? cachedAmount_ : srcSpan.size();

            if (0 == memcmp(srcSpan.data(), cachedData_.data(), cmpLength))
            { // match
                if (cmpLength == srcSpan.size())
                {
                    return {false, true, i};
                }
                if (!fullOnly)
                {
                    return {true, false, i};
                }
            }
            // continue - no match here
        }
        return {false, false, 0};
    }

    /// <summary>
    ///   Sends target to next replacers, and resets partial match index to zero
    /// </summary>
    /// <param name="target">the array we need to send</param>
    void SendAndResetPartialMatch(const span<const char> target) const
    {
        SendFurther(target);
        indexOfPartialMatch_ = 0;
    }

    /// <summary>
    ///   The end of the data sign has been received and the cached data need to be either send or replaced & send
    /// </summary>
    /// <param name="toProcess">character received along with end of data sign</param>
    void DoReplacementsAtTheEndOfTheData(const char toProcess) const
    {
        while (cachedAmount_ > 0)
        {
            const auto [partialMatch, fullMatch, matchPairIndex] = FindMatch(indexOfPartialMatch_, true);
            if (fullMatch)
            {
                const auto& rpair = rpairs_[matchPairIndex];
                SendAndResetPartialMatch(rpair.trg_);
                CleanTheCache(rpair.src_.size());
            }
            else // No full match -> send 1 char from cache
            {
                SendAndResetPartialMatch(std::span<char> (cachedData_.data(), 1));
                CleanTheCache(1);
            }
        }
        pNext_->DoReplacements(toProcess, true);
    }

protected:
    // our pairs sorted by priority - only one of them could be replaced for concrete pos
    vector<ChoiceReplacerPair> rpairs_;

    mutable size_t indexOfPartialMatch_ = 0; // this index from rpairs_ represents last partial match
};

void ChoiceReplacer::DoReplacements(const char toProcess, const bool aEod) const
{
    if (nullptr == pNext_)
    {
        throw logic_error("Replacement chain has been broken. Communicate with maintainer");
    }

    if (aEod) [[unlikely]]
    {
        DoReplacementsAtTheEndOfTheData(toProcess);
        return;
    }

    cachedData_[cachedAmount_++] = toProcess;
    while (cachedAmount_ > 0)
    {
        const auto [partialMatch, fullMatch, matchPairIndex] = FindMatch(indexOfPartialMatch_, false);
        if (fullMatch)
        {
            const auto& rpair = rpairs_[matchPairIndex];
            SendAndResetPartialMatch(rpair.trg_);
            CleanTheCache(rpair.src_.size());
            return;
        }
        if (partialMatch)
        {
            indexOfPartialMatch_ = matchPairIndex;
            return;
        }
        // No any match -> send 1 char from cache
        SendAndResetPartialMatch(std::span<char>(cachedData_.data(), 1));
        CleanTheCache(1);
    }
}

namespace
{
    static std::string_view warningDuplicatePattern("Warning: Duplicate pattern to replace found. Second lexeme will be ignored.");
};

//--------------------------------------------------
/// <summary>
///   replaces for lexemes of the same length
/// </summary>
class UniformLexemeReplacer final : public BaseReplacerWithCache
{
public:
    UniformLexemeReplacer(StreamReplacerChoice& choice, const size_t sz)
    {
        for (AbstractLexemesPair& alpair : choice)
        {
            const span<const char>& src = alpair.first->access();
            const span<const char>& trg = alpair.second->access();
            if (auto result = replaceOptions_.insert(
                {
                    string_view(src.data(), src.size()),
                    string_view(trg.data(), trg.size()),
                }); !result.second)
            {
                cout << coloredconsole::toconsole(warningDuplicatePattern) << endl;
            }
        }
        cachedData_.resize(sz);
    }

    void DoReplacements(const char toProcess, const bool aEod) const override;

protected:
    /// <summary>
    ///   We got the 'end' character so there are no match -> we should pass further all the cache
    /// </summary>
    /// <param name="toProcess">character received along with end of data sign</param>
    void DoReplacementsAtTheEndOfTheData(const char toProcess) const
    {
        SendFurther(std::span<char> (cachedData_.data(), cachedAmount_));
        CleanTheCache(cachedAmount_);
        pNext_->DoReplacements(toProcess, true);
    }
    // here we hold pairs of sources and targets
    unordered_map<string_view, string_view> replaceOptions_;
};


void UniformLexemeReplacer::DoReplacements(const char toProcess, const bool aEod) const
{
    if (nullptr == pNext_)
    {
        throw logic_error("Replacement chain has been broken. Communicate with maintainer");
    }

    // no more data
    if (aEod) [[unlikely]]
    {
        DoReplacementsAtTheEndOfTheData(toProcess);
        return;
    }

    // set buffer of cached at once
    cachedData_[cachedAmount_++] = toProcess;
    if (cachedAmount_ == cachedData_.size())
    {
        if (const auto matchIt = replaceOptions_.find(string_view( cachedData_.data(), cachedAmount_)); matchIt != replaceOptions_.cend())
        {
            string_view trg = matchIt->second;
            SendFurther(trg);
            CleanTheCache(cachedAmount_);
        }
        else
        {
            SendFurther(std::span<char> (cachedData_.data(), 1));
            CleanTheCache(1);
        }
    }
}


//--------------------------------------------------
/// <summary>
///   replaces for lexemes of the same length
/// </summary>
class LexemeOf1Replacer final : public BaseReplacer
{
public:
    LexemeOf1Replacer(StreamReplacerChoice& choice)
    {
        for (AbstractLexemesPair& alpair : choice)
        {
            const size_t index = static_cast<const size_t>(*(reinterpret_cast<const unsigned char*>(alpair.first->access().data())));
            if (replaces_[index].present_)
            {
                cout << coloredconsole::toconsole(warningDuplicatePattern) << endl;
            }
            else
            {
                replaces_[index].present_ = true;
                replaces_[index].trg_ = alpair.second->access();
            }
        }
    }

    void DoReplacements(const char toProcess, const bool aEod) const override;

protected:
    struct
    {
        bool present_ = false; // if this char is present
        span<const char> trg_;
    } replaces_[256];
};


void LexemeOf1Replacer::DoReplacements(const char toProcess, const bool aEod) const
{
    if (nullptr == pNext_)
    {
        throw logic_error("Replacement chain has been broken. Communicate with maintainer");
    }

    // no more data
    if (aEod)
    {
        pNext_->DoReplacements(toProcess, aEod);
        return;
    } // if (aEod)

    const size_t index = static_cast<size_t>(*(reinterpret_cast<const unsigned char*>(&toProcess)));
    if (replaces_[index].present_)
    {
        auto& trg = replaces_[index].trg_;
        SendFurther(trg);
    }
    else
    {
        pNext_->DoReplacements(toProcess, aEod);
    }
}


//--------------------------------------------------
/// <summary>
///   creates replacer for lexemes of the same length
///     since this fact allows to use unordered_map for faster search
/// </summary>
/// <param name="choice">set of pairs of src & trg lexemes - one of which
///   can be processed. The one that was found first.</param>
/// <param name="sz">size of all source lexemes</param>
/// <returns>Replacer for building replacement chain</returns>
unique_ptr<StreamReplacer> CreateEqualLengthReplacer(StreamReplacerChoice& choice, const size_t sz)
{
    if (sz == 1)
    {
        return unique_ptr<StreamReplacer>(new LexemeOf1Replacer(choice));
    }
    return unique_ptr<StreamReplacer>(new UniformLexemeReplacer(choice, sz));
}

//--------------------------------------------------
/// <summary>
///  creates replacer for choose specific lexeme among others to replace
/// </summary>
/// <param name="choice">set of pairs of src & trg lexemes - one of which
///   can be processed. The one that was found first.</param>
/// <returns>Replacer for building replacement chain</returns>
unique_ptr<StreamReplacer> CreateMultipleReplacer(StreamReplacerChoice& choice)
{
    const size_t szSrc = choice.cbegin()->first->access().size(); // save size of the first lexeme

    // check for sources of the same length
    for (const AbstractLexemesPair& alpair: choice)
    {
        if (alpair.first->access().size() != szSrc)
        {
            return unique_ptr<StreamReplacer>(new ChoiceReplacer(choice));
        }
    }

    return CreateEqualLengthReplacer(choice, szSrc); // create optimized replacer for lexemes of the same length
}


//--------------------------------------------------
unique_ptr<StreamReplacer> StreamReplacer::CreateReplacer(StreamReplacerChoice& choice)
{
    for (const AbstractLexemesPair& alpair: choice)
    {
        if (alpair.first->access().empty())
            throw logic_error("Pattern to replace cannot be empty");
    }

    if (choice.size() == 1)
    {
        const AbstractLexemesPair& alexemesPair = *choice.cbegin();
        return CreateSimpleReplacer(alexemesPair.first, alexemesPair.second);
    }
    return CreateMultipleReplacer(choice);
}

}; // namespace bpatch
