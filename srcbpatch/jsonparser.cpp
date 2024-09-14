#include "stdafx.h"
#include "jsonparser.h"

namespace bpatch
{


namespace // for json specific errors
{
    enum : int
    {
        ERR_UEOAF = 0
        , ERR_NOMAINOBJECT = ERR_UEOAF + 1
        , ERR_JSONEXTRA = ERR_NOMAINOBJECT + 1
        , ERR_JSONSTRUCTURE = ERR_JSONEXTRA + 1
        , ERR_JSONNOCOLON = ERR_JSONSTRUCTURE + 1
        , ERR_ENDOFOBJECT = ERR_JSONNOCOLON + 1
        , ERR_NOSTRING = ERR_ENDOFOBJECT + 1
        , ERR_ARRAYVALUE = ERR_NOSTRING + 1
        , ERR_ENDOFARRAY = ERR_ARRAYVALUE + 1
        , ERR_BADSTRING = ERR_ENDOFARRAY + 1
    };
    const char* parsing_errors[] =
    {
        "Unexpected end of Actions file"                      // ERR_UEOAF
        , "Failed to find main object in Actions file"        // ERR_NOMAINOBJECT
        , "Actions file has more characters than expected"    // ERR_JSONEXTRA
        , "Json structure of Actions file is unsuitable"      // ERR_JSONSTRUCTURE
        , "There is no expected ':' character in Actions file"// ERR_JSONNOCOLON
        , "There is unexpected end of object in Actions file" // ERR_ENDOFOBJECT
        , "there is no expected string value in Actions file" // ERR_NOSTRING
        , "Cannot parse array value in Actions file"          // ERR_ARRAYVALUE
        , "There is unexpected end of array in Actions file"  // ERR_ENDOFARRAY
        , "Failed to parse string in Actions file"            // ERR_BADSTRING
    };
    static_assert(ERR_BADSTRING == sizeof(parsing_errors) / sizeof(*parsing_errors) - 1, "You miss some strings!");
}; // nameless namespace - to restrict error access

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////


///                                  LF    CR    Tab
constexpr char whitespaces[] = {' ', 0x0A, 0x0D, 0x09};

bool WhiteSpace(const std::string_view& sv, const size_t at)
{

    return (sv.size() > at &&
        std::ranges::any_of(whitespaces,
        [v = sv.at(at)](const char c)->bool
            { return c == v;}));
}

struct CCpair
{
    char src;
    char trg;
};
constexpr const CCpair controlcharacters[] =
{
    {'\"', '\"'}, {'\\', '\\'}, {'/', '/'},
    {'b', 0x08}, {'f', 0x0C}, {'n', 0x0A},
    {'r', 0x0D}, {'t', 0x09}
};// , 'u'}; /uXXXX ignored


/// <summary>
///   check the symbol at pos to be a Control Character  \" -> '"'
/// </summary>
/// <param name="sv">where to check</param>
/// <param name="at">at pos to check</param>
/// <returns>pos of replacement in controlcharacters
///   or sizeof(controlcharacters) / sizeof(*controlcharacters)</returns>

size_t ControlCharacter(char* sv, const size_t maxLength, const size_t at)
{
    if (at + 1 >= maxLength)
        return std::size(controlcharacters);
    if (sv[at] != '\\')
        return std::size(controlcharacters);

    return std::find_if(std::cbegin(controlcharacters),
        std::cend(controlcharacters),
            [v = sv[at + 1]](const CCpair c)->bool
            { return c.src == v; }) - std::cbegin(controlcharacters);
}


bool ModifyStringValue(char* ptr, size_t& resultLength, const size_t maxLength)
{
    resultLength = 0; // to here set character
    size_t posFromSet = 0; // from here set character
    size_t nReplaces = 0; // how many replaces where done
    while (posFromSet < maxLength)
    {
        if ('\"' == ptr[posFromSet])
            break;
        if ('\0' == ptr[posFromSet])
            return false;

        if (size_t xpos = ControlCharacter(ptr, maxLength, posFromSet);
            xpos != std::size(controlcharacters))
        {
            ptr[resultLength] = controlcharacters[xpos].trg;
            ++posFromSet;
            ++nReplaces;
        }
        else
        {
            ptr[resultLength] = ptr[posFromSet];
        }

        ++posFromSet;
        ++resultLength;
    }

    if (posFromSet >= maxLength)
        return false;

    ptr[resultLength] = '\"';
    for (;nReplaces > 0; --nReplaces)
    {
        ptr[resultLength + nReplaces] = ' ';
    }
    return true;
}

std::pair<bool, size_t> checkJSONPattern(const std::string_view& str, const size_t at, std::regex& patternX)
{
    std::cmatch match;
    const char* const p = str.data() + at;
    if (std::regex_search(p, p + str.size() - at, match, patternX)
        && 0 == match.position())
    {
        return std::make_pair(true, match[0].length());
    }

    return std::make_pair(false, 0);
}


std::pair<bool, size_t> getJSONNumberLength(const std::string_view& str, const size_t at)
{
    std::string_view firstCharSet("-0123456789");
    if (std::ranges::find(firstCharSet, str.at(at)) == std::cend(firstCharSet))
    {
        // first character is not from number
        return std::make_pair(false, 0);
    }

    // JSON number pattern, according to the JSON specification
    std::regex numberPattern(R"(-?(0|([1-9][0-9]*))(.[0-9]+)?([eE][-+]?[0-9]+)?)");

    return checkJSONPattern(str, at, numberPattern);
}


std::pair<bool, size_t> getJSONTrueFalseNull(const std::string_view& str, const size_t at)
{
    
    if (const char c = str.at(at); c != 't' && c!='f' && c != 'n')
    {
        // first character is not from: true false null
        return std::make_pair(false, 0);
    }

    std::regex patternText(R"(true|false|null)");
    return checkJSONPattern(str, at, patternText);
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

// returns true if end of stream reached
bool ThroughWhiteSpaces(std::string_view& sv, size_t& at)
{
    while (at < sv.size() && WhiteSpace(sv, at))
    {
        ++at;
    }
    return at >= sv.size();
}


TJSONObject::PTR_JSON TJSONObject::CreateJSONObject(std::string_view& sv, TJsonCallBack* pCallback)
{
    size_t at = 0;
    PassWhitespaces(sv, at);
    if ('{' != sv.at(at))
    {
        TJSONObject::ReportError(sv, at, parsing_errors[ERR_NOMAINOBJECT]);
    }

    PTR_JSON ret = std::make_shared<TJSONObject>(nullptr);
    ret->callback_ = pCallback;
    ret->OnJSONObject(sv, ++at); // ++at: set at - right after }

    if (sv.size() != at)
    {
        if (!ThroughWhiteSpaces(sv, at))
        {
            TJSONObject::ReportError(sv, at, parsing_errors[ERR_JSONEXTRA]);
        }
    }

    return ret;
}


TJSONObject::TJSONObject(TJSONObject* const aparent)
    : parent_(aparent)
{
    if (parent_)
    {
        callback_ = parent_->callback_;
        level_ = parent_->level_ + 1;
    }
}


size_t TJSONObject::ChildCount() const noexcept
{
    size_t ret = 0;
    do
    {
        // we are in objects
        if (std::holds_alternative<TJSONOBJ_OBJECTS>(containment_))
        {
            const TJSONOBJ_OBJECTS& j_objects = std::get<TJSONOBJ_OBJECTS>(containment_);
            ret = j_objects.size();
            break;
        };

        // in array
        if (std::holds_alternative<TJSONOBJ_ARRAY>(containment_))
        {
            const TJSONOBJ_ARRAY& j_array = std::get<TJSONOBJ_ARRAY>(containment_);
            ret = j_array.size();

            break;
        };

        // value
        if (std::holds_alternative<TJSONOBJ_VALUE>(containment_))
        {
            ret = 1;
            break;
        }

    } while (false);


    return ret;
}


TJSONObject::PTR_JSON TJSONObject::ChildJsonObject(const size_t pos) const noexcept
{
    PTR_JSON ret;
    do
    {
        // we are in objects
        if (std::holds_alternative<TJSONOBJ_OBJECTS>(containment_))
        {
            const TJSONOBJ_OBJECTS& j_objects = std::get<TJSONOBJ_OBJECTS>(containment_);
            if (pos < j_objects.size())
            {
                ret = j_objects.at(pos);
            }

            break;
        };

        // in array
        if (std::holds_alternative<TJSONOBJ_ARRAY>(containment_))
        {
            const TJSONOBJ_ARRAY& j_array = std::get<TJSONOBJ_ARRAY>(containment_);
            if (pos >= j_array.size())
                break;

            const TJSONOBJ_ARRAY_ITEM& aItem = j_array.at(pos);
            if (!std::holds_alternative<PTR_JSON>(aItem))
                break;

            ret = std::get<PTR_JSON>(aItem);
            break;
        };

    } while (false);

    return ret;
}


std::pair<bool, TJSONObject::TJSONOBJ_VALUE> TJSONObject::JsonValue(const size_t pos) const noexcept
{
    bool bSet = false;
    TJSONOBJ_VALUE ret;
    do
    {
        // in array
        if (std::holds_alternative<TJSONOBJ_ARRAY>(containment_))
        {
            const TJSONOBJ_ARRAY& j_array = std::get<TJSONOBJ_ARRAY>(containment_);
            if (pos >= j_array.size())
                break;

            const TJSONOBJ_ARRAY_ITEM& aItem = j_array.at(pos);
            if (!std::holds_alternative<TJSONOBJ_VALUE>(aItem))
                break;

            ret = std::get<TJSONOBJ_VALUE>(aItem);
            bSet = true;
            break;
        };

        // value
        if (std::holds_alternative<TJSONOBJ_VALUE>(containment_))
        {
            ret = std::get<TJSONOBJ_VALUE>(containment_);
            bSet = true;
            break;
        }

    } while (false);

    return {bSet, ret};
}


TJSONObject::PTR_JSON TJSONObject::AddChildWithName(const std::string_view aname)
{
    PTR_JSON pJson;
    do
    {
        // object has been just started
        if (std::holds_alternative<std::monostate>(containment_))
        {
            TJSONOBJ_OBJECTS j_objects;
            pJson = std::make_shared<TJSONObject>(this);
            pJson->name_ = aname;
            j_objects.emplace_back(pJson);
            containment_ = std::move(j_objects);
            break;
        };

        // we are in objects
        if (std::holds_alternative<TJSONOBJ_OBJECTS>(containment_))
        {
            TJSONOBJ_OBJECTS& j_objects = std::get<TJSONOBJ_OBJECTS>(containment_);

            pJson = std::make_shared<TJSONObject>(this);
            pJson->name_ = aname;
            j_objects.emplace_back(pJson);
            break;
        };

        // in array
        // need to get an array
        if (std::holds_alternative<TJSONOBJ_ARRAY>(containment_))
        {
            TJSONOBJ_ARRAY& j_array = std::get<TJSONOBJ_ARRAY>(containment_);

            pJson = std::make_shared<TJSONObject>(this);
            pJson->name_ = aname;
            j_array.emplace_back(pJson);
            break;
        };
    } while(false);

    return pJson;
}


bool TJSONObject::SetValue(std::string_view avalue)
{
    // add into object
    if (std::holds_alternative<std::monostate>(containment_)
        || std::holds_alternative<TJSONOBJ_VALUE>(containment_))
    {
        containment_ = std::move(avalue);
        return true;
    };

    // add into array
    if (std::holds_alternative<TJSONOBJ_ARRAY>(containment_))
    {
        TJSONOBJ_ARRAY& j_array = std::get<TJSONOBJ_ARRAY>(containment_);

        j_array.emplace_back(std::move(avalue));
        return true;
    };
    return false;
}


TJSONObject::PTR_JSON TJSONObject::AddArray()
{
    PTR_JSON pJson;
    do
    {
        if (std::holds_alternative<std::monostate>(containment_))
        {
            // set array type for added object at once
            TJSONOBJ_ARRAY j_array;
            pJson = std::make_shared<TJSONObject>(this);
            j_array.emplace_back(pJson);
            containment_ = std::move(j_array);
            break;
        };

        // add array to array
        if (std::holds_alternative<TJSONOBJ_ARRAY>(containment_))
        {
            TJSONOBJ_ARRAY& j_array = std::get<TJSONOBJ_ARRAY>(containment_);

            pJson = std::make_shared<TJSONObject>(this);
            j_array.emplace_back(pJson);
            break;
        };
    } while(false);

    return pJson;
}


bool TJSONObject::SetMeAsArray()
{
    if (std::holds_alternative<std::monostate>(containment_))
    {
        // set array type for added object at once
        TJSONOBJ_ARRAY j_array;
        containment_ = std::move(j_array);
        return true;
    };
    return false;
}


////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////


/// @brief returns {lines count, pos at line}
auto LinesCount(std::string_view& sv, const size_t till)
{
    size_t n = 0;
    size_t r = 0;
    size_t newLinePos = 0;
    for (size_t i = 0; i < till; ++i)
    {
        const char x = sv.at(i);
        if (0x0a == x)
        {
            newLinePos = i;
            ++n;
        }
        else if (0x0d == x)
        {
            newLinePos = i;
            ++r;
        }
    }
    //                              {lines count,   pos at line}
    return std::pair<size_t, size_t>{1 + (r > n ? r : n), 1 + (till - newLinePos)};
}


void TJSONObject::ReportError(std::string_view& sv, const size_t at, const char* const message)
{
    auto [nLine, posAtLine] = LinesCount(sv, at);
    std::stringstream ss;
    ss << message << " - Line " << nLine << ", at position " << posAtLine <<
        "; Found character '" << sv.at(at) << "';";
    throw std::logic_error(std::move(ss.str()));
}


std::string_view TJSONObject::GetStringValue(std::string_view& sv, size_t& at)
{
    if ('\"' != sv.at(at))
    {
        ReportError(sv, at, parsing_errors[ERR_NOSTRING]);
    }

    // it is beginning of the name
    char* ptr = const_cast<char*>(&sv[++at]);
    // length of the name
    size_t nameLen = 0;
    if (nameLen + at >= sv.size())
    {
        ReportError(sv, nameLen + at - 1, parsing_errors[ERR_UEOAF]);
    }

    size_t rezLength = 0;
    if (!ModifyStringValue(ptr, rezLength, sv.size() - at) || ptr[rezLength] != '\"')
    {
        ReportError(sv, nameLen + at - 1, parsing_errors[ERR_BADSTRING]);
    }
    at += rezLength + 1; // '\"'

    return std::string_view(ptr, rezLength);
}


void TJSONObject::OnJSONObject(std::string_view& sv, size_t& at)
{
    PassWhitespaces(sv, at);
    while (sv.at(at) != '}') // empty object is also ok
    {
        std::string_view aName = GetStringValue(sv, at);
        PTR_JSON pNewJson(AddChildWithName(aName));
        if (pNewJson == nullptr)
        {
            ReportError(sv, at, parsing_errors[ERR_JSONSTRUCTURE]);
        }
        if (callback_)
        {
            callback_->OnJsonObjectBegins(pNewJson.get());
        }

        // 'at' now right after "
        PassWhitespaces(sv, at);
        // 'at' now should points to ':'

        if (':' != sv.at(at))
        {
            ReportError(sv, at, parsing_errors[ERR_JSONNOCOLON]);
        }
        pNewJson->OnJSONValue(sv, ++at); // ++at ':'
        PassWhitespaces(sv, at);

        const char cNext = sv.at(at);
        if ('}' == cNext)
        {
            continue; // end of object
        }

        if (',' == cNext) // if not the last object at this level
        {
            PassWhitespaces(sv, ++at); // ++at ','
            if ('}' == sv.at(at))
            {
                ReportError(sv, at, parsing_errors[ERR_ENDOFOBJECT]);
            }
            continue; // coming to the next object
        };

        ReportError(sv, at, parsing_errors[ERR_JSONSTRUCTURE]);
    }
    ++at; // points after the end of object }
};


void TJSONObject::OnJSONValue(std::string_view& sv, size_t& at)
{
    PassWhitespaces(sv, at);

    do
    {
        const char cNext = sv.at(at);
        if ('\"' == cNext)
        {// string value
            if (!SetValue(GetStringValue(sv, at)))
            {
                ReportError(sv, at, parsing_errors[ERR_JSONSTRUCTURE]);
            }
            // 'at' now right after "
            break;
        }

        if ('{' == cNext)
        {// object value
            PTR_JSON pNewJson(AddChildWithName({}));
            if (pNewJson == nullptr)
            {
                ReportError(sv, at, parsing_errors[ERR_JSONSTRUCTURE]);
            }

            pNewJson->OnJSONObject(sv, ++at); // ++at to pass '{'
            // 'at' now right after {
            break;
        }

        if ('[' == cNext)
        {// object value
            if (!SetMeAsArray())
            {
                ReportError(sv, at, parsing_errors[ERR_JSONSTRUCTURE]);
            }
            OnJSONArray(sv, ++at); // ++at to pass '['
            // 'at' now right after [
            break;
        }

        if (auto [bNumber, numLength] = getJSONNumberLength(sv, at); bNumber)
        {
            SetValue(std::string_view(sv.data() + at, numLength));
            at += numLength;
            break;
        };

        if (auto [bTFN, tfnLength] = getJSONTrueFalseNull(sv, at); bTFN)
        {
            SetValue(std::string_view(sv.data() + at, tfnLength));
            at += tfnLength;
            break;
        };

        ReportError(sv, at, parsing_errors[ERR_JSONSTRUCTURE]);
    } while (false);

    if (callback_) // object just ends
    {
        callback_->OnJsonObjectParsed(this);
    }
}


void TJSONObject::OnJSONArray(std::string_view& sv, size_t& at)
{
    if (callback_)
    {
        callback_->OnJsonArrayBegins(this);
    }

    /// @brief for JSON logic we need to process
    ///    1) array of numbers
    ///    2) array of strings
    ///    3) array of objects
    ///    4) array of arrays
    ///    5) array of mixed [1..4]
    /// 

        // parse only 1 value in array
        // return false if it is not the value at pos 'at'
        // assuming white spaces have been passed already
    auto OnArrayValue = [&]() -> bool
    {
        const char cNext = sv.at(at);
        if ('\"' == cNext)
        {
            if (!SetValue(GetStringValue(sv, at)))
            {
                ReportError(sv, at, parsing_errors[ERR_JSONSTRUCTURE]);
            }
            // at after "
            return true;
        }

        if ('[' == cNext)
        {
            PTR_JSON pNewJson = AddArray();
            if (pNewJson == nullptr)
            {
                ReportError(sv, at, parsing_errors[ERR_JSONSTRUCTURE]);
            }
            pNewJson->OnJSONArray(sv, ++at); // ++at to pass '['
            // 'at' now right after [
            return true;;
        }

        if ('{' == cNext)
        {
            PTR_JSON pNewJson(AddChildWithName({}));
            if (pNewJson == nullptr)
            {
                ReportError(sv, at, parsing_errors[ERR_JSONSTRUCTURE]);
            }

            pNewJson->OnJSONObject(sv, ++at); // ++at to pass '{'
            // 'at' now right after {
            return true;
        }

        if (auto [bNumber, numLength] = getJSONNumberLength(sv, at); bNumber)
        {
            SetValue(std::string_view(sv.data() + at, numLength));
            at += numLength;
            return true;
        };

        if (auto [bTFN, tfnLength] = getJSONTrueFalseNull(sv, at); bTFN)
        {
            SetValue(std::string_view(sv.data() + at, tfnLength));
            at += tfnLength;
            return true;
        };

        return false;
    };

    PassWhitespaces(sv, at);
    while (']' != sv.at(at)) // empty array is also ok
    {
        if (!OnArrayValue())
        {
            ReportError(sv, at, parsing_errors[ERR_ARRAYVALUE]);
        }

        PassWhitespaces(sv, at);
        const char cNext = sv.at(at);
        if (']' == cNext)
        {
            continue; // end of array
        }

        if (',' == cNext)
        {
            PassWhitespaces(sv, ++at); // ++at ','
            if (']' == sv.at(at))
            {
                ReportError(sv, at, parsing_errors[ERR_ENDOFARRAY]);
            }
            continue; // to the next array object
        }

        ReportError(sv, at, parsing_errors[ERR_JSONSTRUCTURE]);
    };
    if (callback_)
    {
        callback_->OnJsonArrayParsed(this);
    }
    ++at; // set after ']'
}


void TJSONObject::PassWhitespaces(std::string_view& sv, size_t& at)
{
    if (ThroughWhiteSpaces(sv, at)) // end of string view has been reached
    {
        ReportError(sv, at, parsing_errors[ERR_UEOAF]);
    }
}


}; // namespace bpatch
