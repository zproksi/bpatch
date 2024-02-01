#include "stdafx.h"
#include <algorithm>
#include "coloredconsole.h"

#ifndef __linux__
#include <windows.h>
#endif


namespace coloredconsole
{
#ifndef __linux__
    /// <summary>
    ///   RAII for set console output color in windows
    /// </summary>
    class WindowsConsoleColor final
    {
        WindowsConsoleColor(const WindowsConsoleColor&) = delete;
        WindowsConsoleColor(WindowsConsoleColor&&) = delete;
        WindowsConsoleColor& operator =(const WindowsConsoleColor&) = delete;
        WindowsConsoleColor& operator =(WindowsConsoleColor&&) = delete;
    public:
        /// <summary>
        ///   set color
        /// </summary>
        /// <param name="color">color to set</param>
        WindowsConsoleColor(WORD color): hConsole_(GetStdHandle(STD_OUTPUT_HANDLE))
        {
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            GetConsoleScreenBufferInfo(hConsole_, &csbi);
            originalColor_ = csbi.wAttributes;
            SetConsoleTextAttribute(hConsole_, color);
        }

        /// <summary>
        ///   set color back
        /// </summary>
        ~WindowsConsoleColor()
        {
            SetConsoleTextAttribute(hConsole_, originalColor_);
        }

    protected:
        HANDLE hConsole_;
        WORD originalColor_;
    };
#endif

    void ColorizeWords(std::ostream& os, const std::string_view& sv)
    {
        static const std::string_view errorSV = "ERROR";
        static const std::string_view warningSV = "Warning";

        size_t pos = 0;
        while (pos < sv.size())
        {
            size_t errorPos = sv.find(errorSV, pos);
            size_t warningPos = sv.find(warningSV, pos);
            if (errorPos != std::string_view::npos && (warningPos == std::string_view::npos || errorPos < warningPos))
            {
                os << sv.substr(pos, errorPos - pos);

#ifdef __linux__
                os << "\033[1;31m" << errorSV << "\033[0m";
#else
                WindowsConsoleColor color(FOREGROUND_RED | FOREGROUND_INTENSITY);
                os << errorSV;
#endif

                pos = errorPos + errorSV.size();
                continue;
            }

            if (warningPos != std::string_view::npos)
            {
                os << sv.substr(pos, warningPos - pos);

#ifdef __linux__
                os << "\033[1;33m" << warningSV << "\033[0m";
#else
                WindowsConsoleColor color(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                os << warningSV;
#endif

                pos = warningPos + warningSV.size();
                continue;
            }

            os << sv.substr(pos, sv.size() - pos);
            pos = sv.size();
        } // while (pos < sv.size())

    }// void ColorizeWords(std::ostream& os, const std::string_view& sv)

} // namespace coloredconsole

