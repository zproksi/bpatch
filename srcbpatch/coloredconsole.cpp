#include "stdafx.h"
#include <algorithm>
#include "coloredconsole.h"

#ifndef __linux__
#include <windows.h>
#endif


namespace coloredconsole
{
#ifndef __linux__
    class WindowsConsoleColor
    {
    public:
        WindowsConsoleColor(WORD color): hConsole(GetStdHandle(STD_OUTPUT_HANDLE))
        {
            GetConsoleScreenBufferInfo(hConsole, &csbi);
            originalColor = csbi.wAttributes;
            SetConsoleTextAttribute(hConsole, color);
        }

        ~WindowsConsoleColor()
        {
            SetConsoleTextAttribute(hConsole, originalColor);
        }

    private:
        HANDLE hConsole;
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        WORD originalColor;
    };
#endif

    void ColorizeWords(std::ostream& os, const std::string_view& sv)
    {
        static const std::string_view error = "ERROR";
        static const std::string_view warning = "Warning";

        size_t pos = 0;
        while (pos < sv.size())
        {
            size_t errorPos = sv.find(error, pos);
            size_t warningPos = sv.find(warning, pos);
            if (errorPos != std::string_view::npos && (warningPos == std::string_view::npos || errorPos < warningPos))
            {
                os << sv.substr(pos, errorPos - pos);

#ifdef __linux__
                os << "\033[1;31m" << error << "\033[0m";
#else
                WindowsConsoleColor color(FOREGROUND_RED | FOREGROUND_INTENSITY);
                os << error;
#endif

                pos = errorPos + error.size();
                continue;
            }

            if (warningPos != std::string_view::npos)
            {
                os << sv.substr(pos, warningPos - pos);

#ifdef __linux__
                os << "\033[1;33m" << warning << "\033[0m";
#else
                WindowsConsoleColor color(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                os << warning;
#endif

                pos = warningPos + warning.size();
                continue;
            }

            os << sv.substr(pos, sv.size() - pos);
            pos = sv.size();
        }
    }// void ColorizeWords(std::ostream& os, const std::string_view& sv)

} // namespace coloredconsole

