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
    WriterReplacer(Writer* const pWriter): pWriter_(pWriter){};

    virtual void DoReplacements(const char toProcess, const bool aEod) const override;
    virtual void SetLastReplacer(std::unique_ptr<StreamReplacer>&& pNext) override;
protected:
    Writer* const pWriter_;
};


void WriterReplacer::DoReplacements(const char toProcess, const bool aEod) const
{
    pWriter_->WriteCharacter(toProcess, aEod);
}


void WriterReplacer::SetLastReplacer(std::unique_ptr<StreamReplacer>&&)
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
    void SetLastReplacer(std::unique_ptr<StreamReplacer>&& pNext) override
    {
        if (pNext_)
        {
            pNext_->SetLastReplacer(std::move(pNext));
        }
        else
        {
            pNext_ = std::move(pNext);
        }
    }

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


//--------------------------------------------------
class UsualReplacer final : public ReplacerWithNext
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
    const span<const char>& src_; // what to replace
    const span<const char>& trg_; // with what

    mutable size_t cachedAmount_ = 0; // we cached this amount of data

    // this is used to hold temporary data while the logic is 
    // looking for the new beginning of the cached value
    mutable vector<char> cachedData_;
};


void UsualReplacer::DoReplacements(const char toProcess, const bool aEod) const
{
    if (nullptr == pNext_)
    {
        throw logic_error("Replacement chain has been broken. Communicate with maintainer");
    }

    // no more data
    // just send cached amount
    if (aEod)
    {
        for (size_t i = 0; i < cachedAmount_; ++i)
        {
            pNext_->DoReplacements(src_[i], false);
        }
        cachedAmount_ = 0;
        pNext_->DoReplacements(toProcess, true);
        return;
    }

    if (src_[cachedAmount_] == toProcess) // check for match
    {
        if (++cachedAmount_ >= src_.size())
        {// send target - do replacement
            for (size_t q = 0; q < trg_.size(); ++q) { pNext_->DoReplacements(trg_[q], false); }
            cachedAmount_ = 0;
        }
        return;
    }

    // here:   toProcess is not our char
    //         lets check for fast track (255/256 probability)
    if (0 == cachedAmount_)
    {
        pNext_->DoReplacements(toProcess, false);
        return;
    }

    // here:   We have some cached data
    //         at least 1 char need to be send further
    //         remaining cached data including toProcess need to be reprocessed for match

    memcpy(cachedData_.data(), src_.data(), cachedAmount_);
    cachedData_[cachedAmount_++]= toProcess;
    size_t i = 0;
    do
    {
        pNext_->DoReplacements(cachedData_[i++], false); // send 1 byte after another
    } while (0 != memcmp(src_.data(), cachedData_.data() + i, --cachedAmount_));
    // Everything that was needed has already been sent
    // cachedAmount_ is zero or greater
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
class ChoiceReplacer final : public ReplacerWithNext
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
    ///    check for partial or full match of data from cachedData_
    ///       with any of lexemes sequentially
    ///       and provides type of match and index of lexeme pairs if found
    /// </summary>
    /// <param name="indexFrom"> search in pairs from this index</param>
    /// <returns>partial, full, index </returns>
    tuple<bool, bool, size_t> FindMatch(const size_t indexFrom) const
    {
        for (size_t i = indexFrom; i < rpairs_.size(); ++i)
        {
            const auto& srcSpan = rpairs_[i].src_;
            const size_t cmpLength = (srcSpan.size() > cachedAmount_) ? cachedAmount_ : srcSpan.size();

            if (0 == memcmp(srcSpan.data(), cachedData_.data(), cmpLength))
            { // match
                const bool fullMatch = cmpLength == srcSpan.size();
                return {!fullMatch, fullMatch, i};
            }

            // continue - no match here
        }
        return {false, false, 0};
    }

    /// <summary>
    ///    Sends a span to the next Replacement (after source span match, for example)
    /// </summary>
    /// <param name="toSend">this span need to be sent</param>
    inline void SendSpanFurther(const std::span<const char>& toSend) const
    {
        for (const char c : toSend)
        {
            pNext_->DoReplacements(c, false);
        }
    }

    /// <summary>
    ///   remove bytes from cache
    /// </summary>
    /// <param name="sz">how many characters to free from the Cache</param>
    inline void ReleaseTheAmountFromTheCache(const size_t sz) const
    {
        shift_left(cachedData_.data(), cachedData_.data() + cachedAmount_, sz);
        cachedAmount_ -= sz;
    }

    /// <summary>
    ///   The end of the data sign has been received and the cached data need to be either send or replaced & send
    /// </summary>
    /// <param name="toProcess">characters received along with end of data sign</param>
    inline void DoReplacementsAtTheEndOfTheData(const char toProcess) const
    {
        // no more data supposed during this run
        // it is necessary to clean the cache inside this logic
        while (cachedAmount_ > 0)
        {
            // to check if we send something or not
            const size_t initialCached = cachedAmount_;

            // for the rest of pairs
            for (size_t i = indexOfPartialMatch_; i < rpairs_.size(); ++i)
            {
                const auto [partialMatch, fullMatch, matchPairIndex] = FindMatch(i);
                if (fullMatch)
                { // need to send replacement
                    const auto& rpair = rpairs_[matchPairIndex];
                    SendSpanFurther(rpair.trg_); // send target after source match

                    ReleaseTheAmountFromTheCache(rpair.src_.size()); // remove matched source from the cache
                    break;
                }
            }
            indexOfPartialMatch_ = 0; // restart searching from 0 - no more incoming data supposed

            // if replacement did not happen, sending should happen anyway,
            //    because it is the end of the data stream
            if (initialCached == cachedAmount_)
            {
                // send 1 byte
                pNext_->DoReplacements(cachedData_[0], false);
                shift_left(cachedData_.data(), cachedData_.data() + cachedAmount_--, 1);
            }
        };

        pNext_->DoReplacements(toProcess, true); // send end of the data further
    }

protected:
    // our pairs sorted by priority - only one of them could be replaced for concrete pos
    vector<ChoiceReplacerPair> rpairs_;

    mutable size_t cachedAmount_ = 0; // we cache this amount of data
    mutable size_t indexOfPartialMatch_ = 0; // at this index from rpairs_

    // this is used to hold temporary data while the logic is 
    // looking for the new beginning of the cached value
    mutable vector<char> cachedData_;
};


void ChoiceReplacer::DoReplacements(const char toProcess, const bool aEod) const
{
    if (nullptr == pNext_) [[unlikely]]
    {
        throw logic_error("Replacement chain has been broken. Communicate with maintainer");
    }

    if (aEod) [[unlikely]]
    {
        DoReplacementsAtTheEndOfTheData(toProcess);
        return;
    } // if (aEod)


    // set buffer of cached at once
    cachedData_[cachedAmount_++] = toProcess;
    while (cachedAmount_ > 0)
    {
        // for the pairs
        for (size_t i = indexOfPartialMatch_; i < rpairs_.size(); ++i)
        {
            const auto [partialMatch, fullMatch, matchPairIndex] = FindMatch(i);
            if (fullMatch)
            { // need to send replacement
                const auto& rpair = rpairs_[matchPairIndex];
                SendSpanFurther(rpair.trg_);  // send target after source match

                ReleaseTheAmountFromTheCache(rpair.src_.size()); // remove matched source from the cache
                indexOfPartialMatch_ = 0; // start from the very first pair of the lexemes again
                return; // replacement happen
            }

            // for partial match it will be more optimal to start searching from the found index next time
            if (partialMatch)
            {
                indexOfPartialMatch_ = matchPairIndex;
                return; // new candidate for replacement
            }
        }// for  (size_t i = indexOfPartialMatch_; i < rpairs_.size(); ++i)

        // send 1 byte, since no full, no partial match found
        pNext_->DoReplacements(cachedData_[0], false);
        shift_left(cachedData_.data(), cachedData_.data() + cachedAmount_--, 1);
        indexOfPartialMatch_ = 0; // start from the very first pair of the lexemes again
    } // while (cachedAmount_ > 0)

    // cachedAmount_ is zero - nothing to send
}


namespace
{
    static std::string_view warningDuplicatePattern("Warning: Duplicate pattern to replace found. Second lexeme will be ignored.");
};

//--------------------------------------------------
/// <summary>
///   replaces for lexemes of the same length
/// </summary>
class UniformLexemeReplacer final : public ReplacerWithNext
{
public:
    UniformLexemeReplacer(StreamReplacerChoice& choice, const size_t sz)
        : cachedData_(sz)
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
    }

    void DoReplacements(const char toProcess, const bool aEod) const override;

protected:
    // here we hold pairs of sources and targets
    unordered_map<string_view, string_view> replaceOptions_;

    mutable size_t cachedAmount_ = 0; // we cache this amount of data in the cachedData_

    // this is used to hold temporary data while the logic is 
    // looking for the new beginning of the cached value
    mutable vector<char> cachedData_;
};


void UniformLexemeReplacer::DoReplacements(const char toProcess, const bool aEod) const
{
    if (nullptr == pNext_)
    {
        throw logic_error("Replacement chain has been broken. Communicate with maintainer");
    }

    // no more data
    if (aEod)
    {
        if (cachedAmount_ > 0)
        {
            for (size_t q = 0; q < cachedAmount_; ++q) { pNext_->DoReplacements(cachedData_[q], false); }
            cachedAmount_ = 0;
        }
        pNext_->DoReplacements(toProcess, aEod); // send end of the data further
        return;
    } // if (aEod)


    // set buffer of cached at once
    char* const& pBuffer = cachedData_.data();
    pBuffer[cachedAmount_++] = toProcess;
    if (cachedAmount_ >= cachedData_.size())
    {
        if (const auto it = replaceOptions_.find(string_view(pBuffer, cachedAmount_)); it != replaceOptions_.cend())
        { // found
            string_view trg = it->second;
            for (size_t q = 0; q < trg.size(); ++q) { pNext_->DoReplacements(trg[q], false); }
            cachedAmount_ = 0;
        }
        else
        { // not found
            pNext_->DoReplacements(pBuffer[0], false); // send 1 char
            std::shift_left(pBuffer, pBuffer + cachedAmount_--, 1);
        }
    }
}


//--------------------------------------------------
/// <summary>
///   replaces for lexemes of the same length
/// </summary>
class LexemeOf1Replacer final : public ReplacerWithNext
{
public:
    LexemeOf1Replacer(StreamReplacerChoice& choice)
    {
        for (AbstractLexemesPair& alpair : choice)
        {
            const size_t index = static_cast<size_t>(*(reinterpret_cast<const unsigned char*>(alpair.first->access().data())));
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
        for (size_t q = 0; q < trg.size(); ++q) { pNext_->DoReplacements(trg[q], false); }
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
