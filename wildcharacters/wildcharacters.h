#pragma once

#include <memory>
#include <regex>
#include <string>
#include <string_view>

/// support of wild chracters in parameters for console applications 
///

namespace wildcharacters
{
    bool TestExecution(int argc, char* argv[]);

    /// <summary>
    ///   class for processing source file name or sourse mask and destination file (folder) name
    ///  case 1: source file name is exactly name + destination file name is exactly name
    ///  case 2: source file name is exactly name + destination is folder name
    ///  case 3: source file name is name with wild characters + destination is folder name
    ///  case 3.1: source file name is name with wild characters + destination is folder name with the same wild characters
    ///  case 4: source file name is name with wild characters + destination is empty  - overwrite mode
    /// </summary>
    class LookUp final
    {
    public:
        /// <summary>
        ///  Registers source and destination names, check for the cases we can process
        ///     throws logic_error, if set of parameters cannot be processed
        /// </summary>
        /// <param name="src_">source file name, or mask for search; Cannot be empty;
        ///   Only file name might contain wild characters * or ? for searching by mask (not folder);
        ///   </param>
        /// <param name="dst_">target file (folder) name;
        ///   Can be empty or contain the same mask as mask for source file - for overwrite mode;
        ///   Can be folder name - then file name will be the same as source file name, but in the target folder;
        ///   If dst_ contains mask - it must be the same as src_ mask</param>
        /// <returns>false - if the folders for src_ and dest_ are the same and wild characters are being used;
        ///    NOTE: logic for returning false comparing both paths as strings for being equal;
        ///   true - all other cases;
        ///   throws logic_error in case of errorneous set of arguments</returns>
        bool RegisterSourceAndDestination(const std::string_view& src_, const std::string_view& dst_);

        /// <summary>
        ///    get next pair of file names for processing
        /// </summary>
        /// <param name="sourceFile">this string will be filled with next source file name</param>
        /// <param name="destinationFile">this string will be filled with destination file name; or become empty,
        ///   for the case of in place processing</param>
        /// <returns>true if we found next pair for processing; false if no more files for processing</returns>
        bool NextFilenamesPair(std::string& sourceFile, std::string& destinationFile);

        /// <summary>
        ///   generates target filename given full(or partial) path with filename
        ///  and foldername where new file should be
        /// </summary>
        /// <param name="originalFilename">full(or partial) path with filename</param>
        /// <param name="foldername">name of the folder where we should generate new file</param>
        /// <param name="resultFilename">full file name in "foldername" to be filled up</param>
        static void GenerateFilenameInFolder(
            const std::string& originalFilename
            , const std::string& foldername
            , std::string& resultFilename
        );

        /// <summary>
        ///   generates regex from a file name with wild characters
        /// </summary>
        /// <param name="mask">a file name with wild characters</param>
        /// <param name="result">reges variable to fill</param>
        /// <returns>true if mask contains wild chracter(s) '*', '?',
        ///  false if no wild characters found</returns>
        static bool GenerateRegularExpression(const std::string& mask, std::regex& result);

    protected:
        /// <summary>
        ///   the source folder picked at RegisterSourceAndDestination moment
        /// </summary>
        std::string srcFolder_;
        /// <summary>
        ///   source file name or the mask picked at RegisterSourceAndDestination moment
        /// </summary>
        std::string srcMask_;

        /// <summary>
        ///   the destination folder picked at RegisterSourceAndDestination moment
        /// </summary>
        std::string dstFolder_;
        /// <summary>
        ///   the destination file name or the mask picked at RegisterSourceAndDestination moment
        /// </summary>
        std::string dstMask_;

    protected:
        /// <summary>
        ///   using with whild characters and search
        /// </summary>
        std::unique_ptr<std::filesystem::directory_iterator> searchIt_;
        std::regex regexMask_;

        /// <summary>
        ///   switch to true if we have no wild characters and already returned pair for processing
        /// </summary>
        enum LookUpSTATUS : int
        {
            MODE_WILDCHARACTER = 0, // have wild characters in srcMask_
            MODE_SINGLEFILE = 1,    // no wild characters in srcMask_
            MODE_WORK_DONE = 2     // all values have been returned by NextFilenamesPair already
        }
        status_ = MODE_WILDCHARACTER;

    };
}// wildcharacters
