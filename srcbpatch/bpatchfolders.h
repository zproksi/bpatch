#pragma once
#include <filesystem>

namespace bpatch
{
    /// <summary>
    ///   Calculates and returns default folder for Action files,
    ///      platform specific: __linux__ define dependent
    /// </summary>
    /// <returns>path to the current executable +/bpatch_a</returns>
    std::filesystem::path& FolderActions();


    /// <summary>
    ///   Calculates and returns default folder for binary files used in Action files,
    ///      platform specific: __linux__ define dependent
    /// </summary>
    /// <returns>path to the current executable +/bpatch_b</returns>
    std::filesystem::path& FolderBinaryPatterns();
};
