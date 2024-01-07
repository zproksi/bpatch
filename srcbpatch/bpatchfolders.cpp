#include "stdafx.h"
#include "fileprocessing.h"
#include "bpatchfolders.h"

#ifdef __linux__
    #include <unistd.h>
#else
    #include <tchar.h>
    #include <windows.h>
#endif 

namespace bpatch
{

namespace
{
    constexpr const char* const subfolderActions = "bpatch_a";
    static std::filesystem::path folderActions; // where actions are by default

    constexpr const char* const subfolderBinary = "bpatch_b";
    static std::filesystem::path folderBinaryPatterns; // where binary patterns are by default

    /// @brief fill folderActions or folderBinaryPatterns if it is not empty
    std::filesystem::path& SubFolder(std::filesystem::path& toSet, const char* const folderName)
    {
        using namespace std::filesystem;
        if (toSet.empty())
        {
            // get full path for executable
#ifdef __linux__
            char pathBuffer[PATH_MAX] = {0};
            readlink(R"(/proc/self/exe)", pathBuffer, PATH_MAX);
#else
            TCHAR pathBuffer[MAX_PATH] = {0};
            GetModuleFileName(nullptr, pathBuffer, MAX_PATH);
#endif
            // set subfolder
            const path directoryPath = path(pathBuffer).parent_path();
            toSet = directoryPath / folderName;
        }
        return toSet;
    }
}; // namespace


std::filesystem::path& FolderActions()
{
    return SubFolder(folderActions, subfolderActions);
}


std::filesystem::path& FolderBinaryPatterns()
{
    return SubFolder(folderBinaryPatterns, subfolderBinary);
}


}; // namespace bpatch
