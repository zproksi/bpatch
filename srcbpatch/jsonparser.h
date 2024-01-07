#pragma once
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace bpatch
{
/// <summary>
///   check if the symbol is in whitespaces
/// </summary>
/// <param name="sv">where to check</param>
/// <param name="at">pos to check</param>
/// <returns>true if the character is whitespace</returns>
bool WhiteSpace(const std::string_view& sv, const size_t at);


/// <summary>
///   we are after open " character
///     modifying data inside string value according ControlCharacter returns
///     shifting ending " accordingly and adding whitespaces in the end of the data
/// </summary>
/// <param name="ptr">where the string starts</param>
/// <param name="resultLength">length of the result string will be returned here</param>
/// <param name="maxLength"> the full length of the data to process</param>
/// <returns>true if logic performed without errors</returns>
bool ModifyStringValue(char* ptr, size_t& resultLength, const size_t maxLength);


/// <summary>
///   Check if a std::string_view begins with a number formatted according to 
/// JSON rules and returns a std::pair<bool, size_t> as a result,
///  where the first element of the pair is true if a valid number is found 
/// at the beginning and the second element is the length of the number, 
/// or false and 0 if no valid number is found,
/// </summary>
/// <param name="str">string view to search in </param>
/// <param name="at">from pos to check</param>
/// <returns>std::pair<bool, size_t> bool (found / not found) +
/// size_t (result length / 0)</returns>
std::pair<bool, size_t> getJSONNumberLength(const std::string_view& str, const size_t at);


/// <summary>
///   Check if a std::string_view begins with true or false or null
/// case insensitive.
/// returns a std::pair<bool, size_t> as a result,
/// where the first element of the pair is true if substring found
/// at the beginning and the second element is the length of the substring, 
/// or false and 0 if no valid number is found,
/// </summary>
/// <param name="str">string view to search in </param>
/// <param name="at">from pos to check</param>
/// <returns>std::pair<bool, size_t> bool (found / not found) +
/// size_t (result length / 0)</returns>
std::pair<bool, size_t> getJSONTrueFalseNull(const std::string_view& str, const size_t at);

/// forward declaration
/// declared in this file below
class TJsonCallBack;

/// <summary>
/// Class helps organize chierarchy during JSON parsing
///   all members are public for easy access
/// </summary>
class TJSONObject
{
public:
    /// level of object in Json
    ///   0 is for main object
    size_t level_ = 0;
    /// the name of the object
    ///   empty by default. As it will be empty for objects in arrays.
    std::string_view name_;

    // std::string_view                   - for value of the json object
    using TJSONOBJ_VALUE = std::string_view;

    using PTR_JSON = std::shared_ptr<TJSONObject>;
    // std::vector<PTR_JSON>              - for child objects
    //                                    - for array of objects
    using TJSONOBJ_OBJECTS = std::vector<PTR_JSON>;


    // new nameless TJSONObject will be created for array
    // variant<>                          - for array item
    using TJSONOBJ_ARRAY_ITEM = std::variant<std::monostate
        , TJSONOBJ_VALUE
        , PTR_JSON
    >;

    // std::vector<  >                    - for array of objects
    using TJSONOBJ_ARRAY = std::vector<TJSONOBJ_ARRAY_ITEM>;

    // std::monostate for empty json objects
    using TJSONOBJ_ENTITY =
        std::variant<std::monostate
            , TJSONOBJ_VALUE
            , TJSONOBJ_OBJECTS
            , TJSONOBJ_ARRAY
        >;

    // all the data inside
    TJSONOBJ_ENTITY containment_ = std::monostate{};

    // if we have parent
    TJSONObject* parent_ = nullptr;

    // if we need callback
    TJsonCallBack* callback_ = nullptr;

    //----------------------------------------------------------
    ////////////////////////////////////////////////////////////

    /// <summary>
    ///   class Factory for root object
    /// </summary>
    /// <param name="sv">string_view to parse</param>
    /// <param name="pCallback"> if you would like to have notifications during parsing</param>
    /// <note>data in string view will be changed! \n->0x0A  for example;
    /// unicode characters ignored</note>
    /// <returns>root Object. Or throws</returns>
    static PTR_JSON CreateJSONObject(std::string_view& sv, TJsonCallBack* pCallback = nullptr);


    TJSONObject(TJSONObject* const aparent);
public:
    // access methods
    ///@brief get amount of children of this Json Object
    size_t ChildCount() const noexcept;

    ///@brief get child object of this Json Object at pos
    PTR_JSON ChildJsonObject(const size_t pos) const noexcept;

    ///@brief get child object of this Json Object at pos
    std::pair<bool, TJSONOBJ_VALUE> JsonValue(const size_t pos) const noexcept;

protected: // parsing

    /// <summary>
    ///   creates or add new set of json objects
    /// </summary>
    /// <param name="aname"></param>
    /// <returns> if success  - non empty pointer of the just created object </returns>
    PTR_JSON AddChildWithName(const std::string_view aname);

    /// <summary>
    ///    set value for object
    /// </summary>
    /// <param name="avalue"> value for object to set</param>
    /// <returns> true if success; false in case of error of type inside</returns>
    bool SetValue(std::string_view avalue);

    /// <summary>
    ///    creates new Json Object with TJSONOBJ_ARRAY
    /// in containment
    /// </summary>
    /// <returns> if success  - non empty pointer of the just created object </returns>
    PTR_JSON AddArray();

    /// <summary>
    ///   Set inner data to contain TJSONOBJ_ARRAY
    /// </summary>
    /// <returns>true only if it was monostate before</returns>
    bool SetMeAsArray();


/// <summary>
///   throws exception with description where we got an error
///   count lines and position in line - message will contain this information
/// </summary>
/// <param name="sv"> data to read</param>
/// <param name="at"> position where we did not find expected in data</param>
/// <param name="message">Explanatory text of the error</param>
static void ReportError(std::string_view& sv, const size_t at, const char* const message);


/// <summary>
///   returns object name from string view
///     expected that the data from at point is
///     whitespace and then "name". throws in case of error.
/// </summary>
/// @note at will be right after \" character of the name
/// <param name="sv">where to search</param>
/// <param name="at">pos from where to search</param>
/// <returns>only name as string view. the data inside could be trasformed according
/// Control Characters replacement </returns>
static std::string_view GetStringValue(std::string_view& sv, size_t& at);


/// <summary>
///   Need to parse the JSON object
/// </summary>
/// <param name="sv">working with this data</param>
/// <param name="at">pos of first character after '{'</param>
void OnJSONObject(std::string_view& sv, size_t& at);

/// <summary>
///   Parsing the value for Json object
/// </summary>
/// <param name="sv">working with this data</param>
/// <param name="at">pos of first character after ':'</param>
void OnJSONValue(std::string_view& sv, size_t& at);


/// <summary>
///    need to save value for the object
/// Parsing the array and doing this
/// </summary>
/// <param name="sv">working with this data</param>
/// <param name="at">pos of first character after '['</param>
void OnJSONArray(std::string_view& sv, size_t& at);


/// <summary>
///   moves 'at' to the next not whitespace character
/// throws in case of out of range
/// </summary>
/// <param name="sv">where to search</param>
/// <param name="at">moving the position</param>
static void PassWhitespaces(std::string_view& sv, size_t& at);

}; // class TJSONObject


/// <summary>
///    Use it to catch the data from the json
/// </summary>
class TJsonCallBack
{
public:
    /// <summary>
    ///   calls when the the name for object just have got parsed
    /// </summary>
    /// <param name="pJson">this object just begins and its name is inside already</param>
    virtual void OnJsonObjectBegins(TJSONObject* const pJson) = 0;


    /// <summary>
    ///   call at the end of the object
    /// </summary>
    /// <param name="pJson">this object just ends</param>
    virtual void OnJsonObjectParsed(TJSONObject* const pJson) = 0;


    /// <summary>
    ///   calls at the beggining of the array
    /// </summary>
    /// <param name="pJson">this array just started</param>
    virtual void OnJsonArrayBegins(TJSONObject* const pJson) = 0;


    /// <summary>
    ///   calls at the end of the array
    /// </summary>
    /// <param name="pJson">this array just ends</param>
    virtual void OnJsonArrayParsed(TJSONObject* const pJson) = 0;

    virtual ~TJsonCallBack() = default;
};







}; // namespace bpatch
