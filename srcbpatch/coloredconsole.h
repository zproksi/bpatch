#pragma once

#include <iostream>
#include <string>
#include <string_view>

namespace coloredconsole
{
    /// <summary>
    ///   function for actual colorization of ERROR and Warning words
    /// </summary>
    /// <param name="os"> std::cout actually</param>
    /// <param name="sv">string view to search ERROR and Warning words for colorization</param>
    void ColorizeWords(std::ostream& os, const std::string_view& sv);

    template<typename T>
    struct is_char_array : std::false_type {}; // by default the type is not char array

    template<std::size_t N>
    struct is_char_array<char[N]> : std::true_type {}; // this is char array and compiler knows it's size

    template<std::size_t N>
    struct is_char_array<const char[N]> : std::true_type {}; // this is char array and compiler knows it's size

    template<typename T> // not a constant from this moment
    constexpr bool is_char_array_v = is_char_array<std::remove_cvref_t<T>>::value;


    /// <summary>
    ///   colorize ERROR and Warning words with help of this class
    /// </summary>
    /// <typeparam name="T">Type to wrap</typeparam>
    template<typename T>
    class Console {
    public:
        Console(T&& value): value_(std::forward<T>(value)) {}

        /// <summary>
        ///  any wrappwed with Console type will have this operator
        /// </summary>
        friend std::ostream& operator<<(std::ostream& os, const Console& logger)
        {
            using BareT = std::remove_cvref_t<T>;
            if constexpr (is_char_array_v<BareT>)
            {
                constexpr auto length = sizeof(BareT) / sizeof(char) - 1;
                ColorizeWords(os, std::string_view(logger.value_, length));
            }
            else if constexpr (
                std::is_same_v<std::string_view, BareT>
                || std::is_same_v<const std::string_view, BareT>
                )
            {
                ColorizeWords(os, logger.value_);
            }
            else if constexpr (
                std::is_same_v<char*, BareT>
                || std::is_same_v<const char*, BareT>
                || std::is_same_v<const char* const, BareT>
                || std::is_same_v<char* const, BareT>
                )
            {
                ColorizeWords(os, std::string_view(logger.value_, strlen(logger.value_)));
            }
            else if constexpr (
                std::is_same_v<std::string, BareT>
                || std::is_same_v<const std::string, BareT>
                )
            {
                ColorizeWords(os, {logger.value_.c_str(), logger.value_.length()});
            }
            else
            {
                os << logger.value_;
            }
            return os;
        }

    private:
        T value_;
    };

    /// <summary>
    ///  Create Wrapped instance
    /// </summary>
    template<typename T>
    Console<T> toconsole(T&& value) {
        return Console<T>(std::forward<T>(value));
    }

};

