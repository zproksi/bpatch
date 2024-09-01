#include <sstream>
#include "wildcharacters.h"

namespace wildcharacters
{
    /// <summary>
    ///   generates target filename given full(or partial) path with filename
    ///  and foldername where new file should be
    /// </summary>
    /// <param name="originalFilename">full(or partial) path with filename</param>
    /// <param name="foldername">name of the folder where we should generate new file</param>
    /// <param name="resultFilename">full file name in "foldername" to be filled up</param>
    void GenerateFilenameInFolder(const std::string& originalFilename, const std::string& foldername, std::string& resultFilename)
    {
        std::filesystem::path originalPath(originalFilename);

        // Get filename from original path
        std::string filename = originalPath.filename().string();

        // Append filename to the foldername
        std::filesystem::path folderPath(foldername);
        folderPath /= filename;

        // Set resultFilename
        resultFilename = folderPath.string();
    }


    /// <summary>
    ///   generates regex from a file name with wild characters
    /// </summary>
    /// <param name="mask">a file name with wild characters</param>
    /// <param name="result">reges variable to fill</param>
    /// <returns>true if mask contains wild chracter(s) '*', '?',
    ///  false if no wild characters found</returns>
    bool GenerateRegularExpression(const std::string& mask, std::regex& result)
    {
        // Check for wildcard characters
        if (mask.find_first_of("*?") == std::string::npos)
            return false;

        std::string regexStr;

        for (char c : mask) {
            switch (c) {
            case '*':
                regexStr += ".*";
                break;
            case '?':
                regexStr += "[^/]";    // Adjusting handling for '?'
                break;
            case '+':
            case '(':
            case ')':
            case '|':
            case '^':
            case '$':
            case '.':
            case '{':
            case '}':
            case '\\':
                // These are special characters in a regexStr, they need to be escaped
                regexStr += '\\';
            default:
                // Everything else is okay
                regexStr += c;
                break;
            }
        }

        // Assign result
        result = std::regex(regexStr, std::regex::ECMAScript);

        return true;
    }


    bool LookUp::RegisterSourceAndDestination(const std::string_view& src_, const std::string_view& dst_)
    {
        auto initialExtraction = [](const std::string_view& a_, std::string& apath, std::string& aname)
        {
            std::filesystem::path src_path(a_);
            // Extract filename or mask
            aname = src_path.filename().string();
            // Extract the parent path
            apath = src_path.parent_path().string();

            // check path for not contain wild characters
            if (apath.find_first_of("*?") != std::string::npos)
            {
                std::stringstream ss;
                ss << "Path '" << a_ << "'cannot contain wild characters";
                throw std::logic_error(ss.str());
            }
        };

        // get source information
        initialExtraction(src_, srcFolder_, srcMask_);

        // check if the destination is only folder name
        if (dst_.size() > 0)
        {
            std::filesystem::directory_entry dirCheck(dst_);
            std::error_code ec;
            if (dirCheck.is_directory(ec))
            {
                dstFolder_ = dst_;
            }
            else
            {
                initialExtraction(dst_, dstFolder_, dstMask_);
            }
        }

        // check the case if masks are the same
        if (dstMask_.find_first_of("*?") != std::string::npos && (0 != dstMask_.compare(srcMask_)))
        {
            std::stringstream ss;
            ss << "Source file mask '" << srcMask_ << "' and Destination file mask '" << dstMask_
               <<"' are not the same";
            throw std::logic_error(ss.str());
        };

        // decide if the logic is for 1 file or for searching
        status_ = srcMask_.find_first_of("*?") != std::string::npos ? MODE_WILDCHARACTER : MODE_SINGLEFILE;

        // return false if the processing with wild character in the same folder
        // for such case destination parameter can be skipped
        return !(MODE_WILDCHARACTER == status_ && 0 == srcFolder_.compare(dstFolder_));
    }

    bool LookUp::NextFilenamesPair(std::string& sourceFile, std::string& destinationFile)
    {
        if (MODE_WORK_DONE == status_)
            return false; // no more valid data

        if (MODE_SINGLEFILE == status_)
        {
            // get source file name
            GenerateFilenameInFolder(srcMask_, srcFolder_, sourceFile);

            // get destination file name
            if (dstMask_.empty())
            {
                // no explicit destination file name provided
                // could be either inplace processing or file with the same name at the destination folder
                GenerateFilenameInFolder(srcMask_, dstFolder_.empty() ? srcFolder_ : dstFolder_, destinationFile);
            }
            else
            {
                // destination file name was provided
                GenerateFilenameInFolder(dstMask_, dstFolder_, destinationFile);
            }

            status_ = MODE_WORK_DONE;
            return true; // returned pair is valid
        }

        // searching by mask
        // generate regex - get iterator
        if (!searchIt_)
        {
            std::error_code ec;
            searchIt_.reset(new std::filesystem::directory_iterator(std::filesystem::path(srcFolder_) / ".", ec));
            if (ec.value())
            {
                std::stringstream ss;
                ss << "Failed to iterate through folder '" << srcFolder_ << "': " << ec.message();
                throw std::logic_error(ss.str());
            }

            GenerateRegularExpression(srcMask_, regexMask_);
            if (dstFolder_.empty())
            {
                dstFolder_ = srcFolder_; // inplace processing
            }
        }
        std::filesystem::directory_iterator endIt; // end iterator

        while (*searchIt_ != endIt)
        {
            std::filesystem::directory_entry entry(**searchIt_);
            if (entry.is_regular_file()) // regular file
            {
                std::filesystem::path aPotentialSourceFilePath = entry.path();
                const std::string aname = aPotentialSourceFilePath.filename().string();

                if (std::regex_match(aname, regexMask_)) // check by mask
                {
                    sourceFile = aPotentialSourceFilePath.string();
                    GenerateFilenameInFolder(aname, dstFolder_, destinationFile);
                    ++(*searchIt_);
                    return true;
                }
            }

            ++(*searchIt_);
        }

        status_ = MODE_WORK_DONE;
        return false;
    }

}// wildcharacters
