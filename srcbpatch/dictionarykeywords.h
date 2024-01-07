#pragma once

#include <string_view>

namespace bpatch
{
    extern const std::string_view dictionarySV;
    extern const std::string_view hexadecimalSV;
    extern const std::string_view decimalSV;
    extern const std::string_view textSV;
    extern const std::string_view fileSV;
    extern const std::string_view compositeSV;
    extern const std::string_view todoSV;
    extern const std::string_view replaceSV;

    /// <summary>
    ///   empty string view (size == 0)
    /// </summary>
    extern const std::string_view emptySV;
};
