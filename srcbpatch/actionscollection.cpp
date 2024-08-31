#include "stdafx.h"
#include "actionscollection.h"
#include "binarylexeme.h"
#include "dictionary.h"
#include "dictionarykeywords.h"
#include "fileprocessing.h"
#include "jsonparser.h"
#include "bpatchfolders.h"


namespace bpatch
{

// ways to our data
namespace
{

    constexpr const size_t levelBase = 1; // dictionary, todo

    constexpr const size_t levelReplaceIn = 3;

    constexpr const size_t levelReplaceObject = 5;
    constexpr const size_t levelArray = 5;
    constexpr const size_t levelTextObject = 5;
    constexpr const size_t levelFileObject = 5;

///@brief check for "dictionary" or "todo"
/// with help of this function
template <const std::string_view& sv1>
bool JsonAtLevel1(TJSONObject* const pJson)
{
    return (levelBase == pJson->level_ &&
        0 == sv1.compare(pJson->name_));
};

///@brief check the node at level 3 under
///   "dictionary" or "todo"
/// with help of this function
template <const std::string_view& sv1, const std::string_view& sv3>
bool JsonAtLevel1_3(TJSONObject* const pJson)
{
    return (3 == pJson->level_ &&
        0 == sv3.compare(pJson->name_) &&
        JsonAtLevel1<sv1>(pJson->parent_->parent_));
};


///@brief check the node at level 3 under
///   "dictionary"
/// with help of this function
template <const std::string_view& sv3>
bool JsonDictLevel3(TJSONObject* const pJson)
{
    return (3 == pJson->level_ &&
        0 == sv3.compare(pJson->name_) &&
        JsonAtLevel1<dictionarySV>(pJson->parent_->parent_));
};



/// <summary>
///   check the node with decimal arrays
/// </summary>
/// <param name="pJson">node to check</param>
/// <returns>true if it is the node with decimal numbers</returns>
bool IsDecimalArray(TJSONObject* const pJson)
{
    return (levelArray == pJson->level_ &&
        nullptr != pJson->name_.data() && // valid name
        JsonDictLevel3<decimalSV>(pJson->parent_->parent_));
}


/// <summary>
///   check the node with decimal arrays
/// </summary>
/// <param name="pJson">node to check</param>
/// <returns>true if it is the node with hexadecimal numbers</returns>
bool IsHexadecimalArray(TJSONObject* const pJson)
{
    return (levelArray == pJson->level_ &&
        nullptr != pJson->name_.data() && // valid name
        JsonDictLevel3<hexadecimalSV>(pJson->parent_->parent_));
}


/// <summary>
///   check the node with composites
/// </summary>
/// <param name="pJson">node to check</param>
/// <returns>true if it is the node with composite data</returns>
bool IsCompositeArray(TJSONObject* const pJson)
{
    return (levelArray == pJson->level_ &&
        nullptr != pJson->name_.data() && // valid name
        JsonDictLevel3<compositeSV>(pJson->parent_->parent_));
}


/// <summary>
///   check the node with text
/// </summary>
/// <param name="pJson">node to check</param>
/// <returns>true if it is the node with texts</returns>
bool IsTextObject(TJSONObject* const pJson)
{
    return (levelTextObject == pJson->level_ &&
        nullptr != pJson->name_.data() && // valid name
        JsonDictLevel3<textSV>(pJson->parent_->parent_));
}


/// <summary>
///   check the node with files
/// </summary>
/// <param name="pJson">node to check</param>
/// <returns>true if it is the node with file names</returns>
bool IsFileObject(TJSONObject* const pJson)
{
    return (levelFileObject == pJson->level_ &&
        nullptr != pJson->name_.data() && // valid name
        JsonDictLevel3<fileSV>(pJson->parent_->parent_));
}


/// <summary>
///    check the node with replace description
/// </summary>
/// <param name="pJson">node to check</param>
/// <returns>true if it is the node with decimal numbers</returns>
bool IsReplaceObject(TJSONObject* const pJson)
{
    return (levelReplaceObject == pJson->level_ &&
        nullptr != pJson->name_.data() && // valid name
        JsonAtLevel1_3<todoSV, replaceSV>(pJson->parent_->parent_));
}

}; // nameless namespace


ActionsCollection::ActionsCollection(std::vector<char>&& dataSource)
    : jsondata_(std::move(dataSource))
    , readdata_(static_cast<std::vector<char>::size_type>(SZBUFF_FC))
{
    std::string_view srcView(jsondata_.data(), jsondata_.size());

    [[maybe_unused]] TJSONObject::PTR_JSON everything = TJSONObject::CreateJSONObject(srcView, this);

    ProcessComposites();

    // setup replacers chain
    CreateChainOfReplacers();
}


void ActionsCollection::OnJsonObjectBegins(TJSONObject* const pJson)
{
    if (0 == replaceSV.compare(pJson->name_) && pJson->level_ == levelReplaceIn)
    {
        replaces_.emplace_back(VectorStringviewPairs());
    }
}


void ActionsCollection::OnJsonObjectParsed(TJSONObject* const pJson)
{
    if (IsTextObject(pJson))
    {
        auto [exists, value] = pJson->JsonValue(0);
        if (!exists)
        {
            ReportError("Expecting string value in text objects of dictionary");
        }

        if (const bool added = dictionary_.AddBinaryLexeme(pJson->name_,
                AbstractBinaryLexeme::LexemeFromStringView(value));
            !added)
        {// overwritten
            ReportDuplicateNameError(pJson->name_);
        }
        return;
    }

    if (IsFileObject(pJson))
    {
        using namespace std;
        auto [exists, fileName] = pJson->JsonValue(0);
        if (!exists)
        {
            ReportError("Expecting names in file objects of dictionary");
        }
        // load file
        // 
        // set string with file name - zero terminated
        // to use in logic
        const_cast<char*>(fileName.data())[fileName.size()] = '\0';

        // read file data
        vector<char> adata;
        if (!ReadFullFile(adata, fileName.data(), FolderBinaryPatterns()))
        {
            stringstream ss;
            ss << "Failed to read file '" << fileName << "' which has been mentioned in Actions file";
            throw logic_error(ss.str().c_str());
        }

        // create binary lexeme from readed data
        if (const bool added = dictionary_.AddBinaryLexeme(pJson->name_,
            AbstractBinaryLexeme::LexemeFromVector(move(adata)));
            !added)
        {// overwritten
            ReportDuplicateNameError(pJson->name_);
        }
        return;
    }

    if (IsReplaceObject(pJson))
    {
        auto [exists, value] = pJson->JsonValue(0);
        if (!exists)
        {
            ReportError("Expecting string in todo array objects of dictionary");
        }
        replaces_.rbegin()->emplace_back(
            std::pair<std::string_view, std::string_view>(pJson->name_, value));
        return;
    }
}


void ActionsCollection::OnJsonArrayBegins(TJSONObject* const)
{
}


void ActionsCollection::OnJsonArrayParsed(TJSONObject* const pJson)
{
    if (IsDecimalArray(pJson))
    {
        std::string_view value =
            ParseDictionaryArray(pJson, 10,
                "Expecting only values [0..255] in \"decimal\" dictionarys");

        if (const bool added = dictionary_.AddBinaryLexeme(pJson->name_,
            AbstractBinaryLexeme::LexemeFromStringView(value));
            !added) [[unlikely]]
        {// overwritten
            ReportDuplicateNameError(pJson->name_);
        }
        return;
    }

    if (IsHexadecimalArray(pJson))
    {
        std::string_view value =
            ParseDictionaryArray(pJson, 16,
                "Expecting only string values [00..FF] in \"hexadecimal\" dictionarys");

        if (const bool added = dictionary_.AddBinaryLexeme(pJson->name_,
            AbstractBinaryLexeme::LexemeFromStringView(value));
            !added) [[unlikely]]
        {// overwritten
            ReportDuplicateNameError(pJson->name_);
        }
        return;
    }

    if (IsCompositeArray(pJson))
    {
        composites_.emplace_back(pJson->name_, ParseArray(pJson, "Composite lexemes can contain only names"));
        return;
    }
}


void ActionsCollection::DoReplacements(const char toProcess, const bool aEod) const
{
    // writing bytes - ActionsCollection is last in the replacers chain
    pWriter_->WriteCharacter(toProcess, aEod);
}


void ActionsCollection::SetLastReplacer(std::unique_ptr<StreamReplacer>&&)
{
    throw std::logic_error("Writer Replacer should be unchangeable. Contact with maintainer.");
}


void ActionsCollection::DoReadReplaceWrite(Reader* const pReader, Writer* const pWriter)
{
    pWriter_ = pWriter; // set pWriter for output - class being setup as last in the chain
    using namespace std;
    do
    {
        auto fullSpan = pReader->ReadData(span{readdata_.data(), SZBUFF_FC}); // read data
        ranges::for_each(fullSpan,
            [this](const char c)
            {
                replacersChain_->DoReplacements(c, false); // process data
            }
        );

    } while (!pReader->FileReaded());
    replacersChain_->DoReplacements('e', true); // only 'true' as sign of data end is important here
}


void ActionsCollection::ReportError(const char* const message)
{
    throw std::runtime_error(message);
}

void ActionsCollection::ReportDuplicateNameError(const std::string_view& aname)
{
    std::stringstream ss;
    ss << "Duplicate name '" << aname << "' has been met in Actions file";
    ReportError(ss.str().c_str());
}


void ActionsCollection::ReportMissedNameError(const std::string_view& aname)
{
    std::stringstream ss;
    ss << "Cannot find lexeme name '" << aname << "' in Actions file";
    ReportError(ss.str().c_str());
}


std::string_view ActionsCollection::ParseDictionaryArray(TJSONObject* const pJson, const int base, const char* const errorMsg)
{
    // here we will hold result string view
    // actuallly since we are working with underlying jsondata_
    // we will modify data of json text in this logic
    // to form necessary string view
    char* pTarget = nullptr;
    size_t rezLength = 0;

    const size_t sz = pJson->ChildCount();
    if (sz == 0) [[unlikely]]// empty array
    {
        return emptySV; // empty data - can be used to remove something
    }

    for (size_t i = 0; i < sz; ++i)
    {
        auto [exists, value] = pJson->JsonValue(i);
        if (!exists)
        {
            pTarget = nullptr;
            break; // not a string received but object
        }

        if (0 == i)
        { // save the pointer on first element - here we will accumulate the data
            pTarget = const_cast<char*>(value.data());
        }

        unsigned char c = 0;
        std::from_chars_result rez =
            std::from_chars(value.data(), value.data() + value.length(),
                c, base);
        if (rez.ec != std::errc())
        { // we got something which is not a number
            pTarget = nullptr;
            break;
        }
        // add a value to the result string view
        pTarget[rezLength++] = *reinterpret_cast<char*>(&c);
    } // for

    if (nullptr == pTarget) [[unlikely]]
    {
        ReportError(errorMsg); // throws
        return emptySV;
    }

    return std::string_view(pTarget, rezLength);
}


std::vector<std::string_view> ActionsCollection::ParseArray(TJSONObject* const pJson, const char* const errorMsg)
{
    const size_t sz = pJson->ChildCount();
    if (sz == 0) // empty array
    {
        ReportError(errorMsg);
    }

    std::vector<std::string_view> ret;
    for (size_t i = 0; i < sz; ++i)
    {
        auto [exists, value] = pJson->JsonValue(i);
        if (!exists)
        {
            ReportError(errorMsg);// not a string received but object
        }
        ret.emplace_back(std::move(value));
    }

    return ret;
}


void ActionsCollection::ProcessComposites()
{
    for (const auto& [name, vectorData]: composites_)
    {// for every composite lexeme

        std::unique_ptr<AbstractBinaryLexeme> compositeLexeme;
        size_t totalLength = 0;
        for (const auto& subname : vectorData)
        {
            // get existent lexeme
            auto& existantLexeme = dictionary_.Lexeme(subname);
            if (existantLexeme == nullptr)
            {
                ReportMissedNameError(subname);
            }
            const size_t len = existantLexeme->access().size();

            if (compositeLexeme == nullptr) // just created
            {
                compositeLexeme = 
                    AbstractBinaryLexeme::LexemeFromLexeme(existantLexeme);
                totalLength = len;
                continue;
            }

            // fill data for composite lexeme
            compositeLexeme->Replace(totalLength, 0, existantLexeme);
            totalLength += len;
        }

        // now move newly created lexeme into Dictionary
        if (const bool added = 
            dictionary_.AddBinaryLexeme(name, std::move(compositeLexeme));
            !added)
        {// overwritten
            ReportDuplicateNameError(name);
        }
    }

    // everything has been processd.
    // free unnecessary collection
    composites_.clear();
}

//--------------------------------------------------
/// <summary>
///   Replacer to be the last in the chain
/// All incoming data should be trasferred to the ActionsCollection class
/// ActionsCollection class will transfer the data to the Writer interface
/// </summary>
class LastReplacer final : public StreamReplacer
{
    ActionsCollection* const pac_;
public:
    LastReplacer(ActionsCollection* const pac): pac_(pac)
    {
    }

    virtual void DoReplacements(const char toProcess, const bool aEod) const override
    {
        pac_->DoReplacements(toProcess, aEod);
    }

    virtual void SetLastReplacer(std::unique_ptr<StreamReplacer>&& pNext) override
    {
        pac_->SetLastReplacer(std::move(pNext));
    }
};
//--------------------------------------------------


void ActionsCollection::CreateChainOfReplacers()
{
    if (replaces_.empty())
    {
        ReportError("Nothing to replace in todo array of Actions file");
    }

    // setup LastReplacer in the end of the replacers chain at once
    // `this` passed inside as a copy. ActionsCollection owns whole replacers chain
    replacersChain_.reset(new LastReplacer(this));

    for (auto rit = replaces_.crbegin(); rit != replaces_.crend(); ++rit) // from the end
    {
        const VectorStringviewPairs& vPairs = *rit;

        if (vPairs.empty())// check for no replace
        {
            // replacements not found - actually good place for warning
            std::cout << coloredconsole::toconsole("Warning: empty json \"replace\" object detected.") << std::endl;
            continue;
        }

        // multiple replacer creation place
        StreamReplacerChoice sourceTargetPairs;
        for (VectorStringviewPairs::const_iterator itPair = vPairs.cbegin();
            itPair != vPairs.cend(); ++itPair)
        {
            const auto& vpair = *itPair;
            auto alexemesPair = dictionary_.LexemesPair(vpair.first, vpair.second);
            if (alexemesPair.first == nullptr)
            {
                ReportMissedNameError(vpair.first);
            }
            if (alexemesPair.second == nullptr)
            {
                ReportMissedNameError(vpair.second);
            }

            sourceTargetPairs.emplace_back(std::move(alexemesPair));
        }
        // create replacer
        std::unique_ptr<StreamReplacer> replacer = StreamReplacer::CreateReplacer(sourceTargetPairs);
        // `replacer` needs to hold tail of the chain
        // replacersChain_ contains the tail of chain
        replacer->SetLastReplacer(std::move(replacersChain_)); // now full chain is in replacer
        replacersChain_ = std::move(replacer); // now full chain is in place
    } // for(auto rit = replaces_.rbegin(); rit != replaces_.rend(); ++rit)

    // everything has been created. free some memory
    replaces_.clear();
}

};// namespace bpatch
