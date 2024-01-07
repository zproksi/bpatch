#include "dictionarykeywords.h"

namespace bpatch
{
    /// @brief literals usage
    using namespace std::literals;
    constexpr std::string_view dictionarySV("dictionary"sv);
    constexpr std::string_view hexadecimalSV = "hexadecimal"sv;
    constexpr std::string_view decimalSV{"decimal"sv};

    /// @brief usual initialization
    constexpr std::string_view textSV{"text"};
    constexpr std::string_view fileSV{"file"};
    constexpr std::string_view compositeSV{"composite"};
    constexpr std::string_view todoSV{"todo"};
    constexpr std::string_view replaceSV{"replace"};

    constexpr std::string_view emptySV(textSV.data(), 0);

};
