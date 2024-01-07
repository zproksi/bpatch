#pragma once
// #include "actionscollection.h"
#include <string_view>
#include <list>
#include <memory>
#include <vector>

#include "dictionary.h"
#include "jsonparser.h"

namespace bpatch
{
/// <summary>
///    class contains main entry points for processing.
///    * JSON parser callback to load settings for processing
///    * pipeline to process binary lexemes
/// </summary>
class ActionsCollection final: public TJsonCallBack, public StreamReplacer
{
public:
    /// <summary>
    ///   constructor accepts vector with json data and process it
    /// </summary>
    /// <param name="dataSource">json data - vecor data will be held inside and modified
    ///   for inner simplicity of usage</param>
    /// <param name="pLast">StreamReplacer to pass result of actions 
    ///   in the end of processing</param>
    ActionsCollection(std::vector<char>&& dataSource);

    /// <summary>
    ///  callback from TJsonCallBack
    /// </summary>
    /// <param name="pJson">Json object just got its name in parser</param>
    void OnJsonObjectBegins(TJSONObject* const pJson) override;

    /// <summary>
    ///  callback from TJsonCallBack
    /// </summary>
    /// <param name="pJson">Json object just ends in parser</param>
    void OnJsonObjectParsed(TJSONObject* const pJson) override;

    /// <summary>
    ///  callback from TJsonCallBack
    /// </summary>
    /// <param name="pJson">Json array just begins in parser</param>
    void OnJsonArrayBegins(TJSONObject* const pJson) override;

    /// <summary>
    ///  callback from TJsonCallBack
    /// </summary>
    /// <param name="pJson">Json array just ends in parser</param>
    void OnJsonArrayParsed(TJSONObject* const pJson) override;

    /// <summary>
    ///   callback from StreamReplacer
    /// </summary>
    /// <param name="toProcess">this char is under processing right now</param>
    /// <param name="aEod">this is sign that no more data</param>
    void DoReplacements(const char toProcess, const bool aEod) const override;

    /// <summary>
    ///   callback from StreamReplacer
    /// </summary>
    /// <param name="toProcess">replacer to call next</param>
    void SetNextReplacer(StreamReplacer* const pNext) override;


protected:
    /// <summary>
    ///   throws error if we meet error in the expected logic
    /// </summary>
    /// <param name="message">text of the error</param>
    void ReportError(const char* const message);

    /// <summary>
    ///   throws error if we meet duplicate name of lexeme
    /// </summary>
    /// <param name="aname">duplicate name of lexeme</param>
    void ReportDuplicateNameError(const std::string_view& aname);

    /// <summary>
    ///   throws error if we cannot find lexeme for composite lexeme
    /// </summary>
    /// <param name="aname">nonexistent name of lexeme</param>
    void ReportMissedNameError(const std::string_view& aname);

    /// <summary>
    ///   parse array of either 10 based integers or array of hex based strings
    /// </summary>
    /// <param name="pJson">array is inside</param>
    /// <param name="base">is the base 10 or 16 - set 10 or 16 here</param>
    /// <param name="errorMsg">error mesage for the case of parsing error</param>
    /// <returns>result string view with data
    ///     data will be located in memory of main json vector (data_)</returns>
    std::string_view ParseDictionaryArray(TJSONObject* const pJson, const int base, const char* const errorMsg);

    /// <summary>
    ///     to parse arrays (composite)
    /// </summary>
    /// <param name="pJson">array with strings is here</param>
    /// <param name="errorMsg">in case we have an object inside. or the result has no data</param>
    /// <returns>vector with string_views</returns>
    std::vector<std::string_view> ParseArray(TJSONObject* const pJson, const char* const errorMsg);

    /// <summary>
    ///    we are creating and registering composite lexemes inside
    ///  we are finalizing initialization of dictionary
    /// </summary>
    void ProcessComposites();

    /// <summary>
    ///    we are creating and making chain of replacers
    ///   last operation in initialization - dictionary is 100% ready
    /// </summary>
    void CreateChainOfReplacers();

protected:
    // this is data for json parsing
    // all lexemes from json file is inside
    std::vector<char> data_;

    // here we hold all lexemes from our json
    Dictionary dictionary_;

    /// intermediate collections
    /// since we are not expecting ordered data from json
    ///  = composite sets could be first, we accumulate composites. Than will parse
    std::vector<std::pair<std::string_view, std::vector<std::string_view>>> composites_;

    // replaces pairs
    // need to be processed last
    typedef std::pair<std::string_view, std::string_view> StringviewPair; // src + trg
    typedef std::vector<StringviewPair> VectorStringviewPairs; // all pairs from one replace
    std::vector<VectorStringviewPairs> replaces_; // all replaces

    /// <summary>
    ///  here we are holding chain of the replacers
    /// </summary>
    std::list<std::unique_ptr<StreamReplacer>> replacersChain_;
}; // class ActionsCollection


};// namespace bpatch
