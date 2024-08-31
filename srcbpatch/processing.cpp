#include "stdafx.h"
#include "actionscollection.h"
#include "binarylexeme.h"
#include "bpatchfolders.h"
#include "consoleparametersreader.h"
#include "fileprocessing.h"
#include "processing.h"
#include "timemeasurer.h"
#include "wildcharacters.h"


namespace bpatch
{
using namespace std;

namespace
{
    struct ProcessingInfo
    {
        string_view file_source = "";
        string_view file_target = "";
        string_view file_actions = "";
        bool overwrite = false;
    };

    struct FileProcessingInfo
    {
        unique_ptr<ActionsCollection>& todo;
        string& src;
        string& dst;
        const bool overwrite;
        size_t readed;
        size_t written;
    };

};


unique_ptr<ActionsCollection> CreateActionsFile(string_view actionsFileName)
{
    vector<char> adata;
    if (!ReadFullFile(adata, actionsFileName.data(), FolderActions()))
    {
        throw logic_error("Failed to read Actions file as one chunk.");
    }

    // Parsing of todo and lexemes
    // Dictionary will be inside
    return unique_ptr<ActionsCollection>(new ActionsCollection(move(adata)));
}


/// <summary>
///   Deside if the file will be processed inplace or as source + target
/// Creates Reader and Writer. And proceed futher to DoReadReplaceWrite
/// </summary>
/// <param name="jobInfo">description of the files pair and todo object</param>
/// <returns>true if actual processing happend</returns>
bool ProcessTheFile(FileProcessingInfo& jobInfo)
{
    using namespace std;
    /// --------------------------------------------------------
    /// if source and target file are the same
    /// -- processing inplace --
    /// 
    if (0 == jobInfo.src.compare(jobInfo.dst))
    {
        {
            ReadWriteFileProcessing rwProcessing(jobInfo.src.c_str());
            jobInfo.todo->DoReadReplaceWrite(&rwProcessing, &rwProcessing);
            jobInfo.written = rwProcessing.Written();
            jobInfo.readed = rwProcessing.Readed();
        } // close file

        // set file size
        // because we can write less than read
        filesystem::resize_file(jobInfo.src.c_str(), jobInfo.written);
        return true; // inplace processing has been done
    }


    /// -------------------------------------------------------
    /// source and target are different files
    /// -- processing reading and writing in different files --
    /// 
    error_code ec;
    if (!jobInfo.overwrite &&
        filesystem::exists(jobInfo.dst, ec))
    { // check override possibility
        cout << coloredconsole::toconsole("Warning: Target file '") << jobInfo.dst << "' exists. "
            "Use /w instead of /t to overwrite.\n Processing skipped\n";
        jobInfo.written = 0;
        jobInfo.readed = 0;
        return false;
    }

    ReadFileProcessing reader(jobInfo.src.c_str());
    WriteFileProcessing writer(jobInfo.dst.c_str());

    jobInfo.todo->DoReadReplaceWrite(&reader, &writer);
    // we do not resize file here because we have opened/created file only for writing
    jobInfo.written = writer.Written();
    jobInfo.readed = reader.Readed();

    return true;
}


/// <summary>
///  Wild charactes processing level.
/// ActionsCollection will be created here
///   By Mask - means file masked by either '?' or '*' or both in any combination
///     jobInfo provides result names
/// </summary>
/// <param name="jobInfo">parameters from command string</param>
/// <returns>true; or throws</returns>
bool ProcessFilesByMask(ProcessingInfo& jobInfo)
{
    using namespace std;
    cout << "Actions file:         '" << jobInfo.file_actions << "'\n";

    /// --------------------------------------------------------
    /// load Actions and initialize processing class
    /// Json parsing is inside
    unique_ptr<ActionsCollection> todo = CreateActionsFile(jobInfo.file_actions);

    // look up logic for files
    wildcharacters::LookUp lookupMasks; // masked files from command line
    lookupMasks.RegisterSourceAndDestination(jobInfo.file_source, jobInfo.file_target);
    string srcFilename; // source file name
    string dstFilename; // destination file name

    size_t filesProcessed = 0;
    FileProcessingInfo fileInfo{.todo = todo, .src = srcFilename, .dst = dstFilename, .overwrite = jobInfo.overwrite};
    while (lookupMasks.NextFilenamesPair(srcFilename, dstFilename)) // request file names
    {
        cout << "Source file:          '" << fileInfo.src << "'\n";
        cout << "Target file:          '" << fileInfo.dst << "'\n";
        if (bpatch::ProcessTheFile(fileInfo))
        {
            ++filesProcessed;
        }

        cout << "Readed (bytes):       '" << fileInfo.readed << "'\n";
        cout << "Written (bytes):      '" << fileInfo.written << "'\n";
        cout << "\n";
    };
    cout << "Files processed:      '" << filesProcessed << "'\n";

    return true;
}

namespace
{
    bpatch::ConsoleParametersReader parametersReader;
};


/// <summary>
///   Entry point of library
/// All exceptions handling must be only here
/// </summary>
bool Processing(int argc, char* argv[])
{
    using namespace coloredconsole;

    TimeMeasurer fulltime("Processing took");
    if (!parametersReader.ReadConsoleParameters(argc, argv))
    {
        cout << parametersReader.Manual();
        return false;
    }

    int retValue = false;
    try
    {
        cout << "\n";
        cout << "Executable:           '" << argv[0] << "'\n";
        cout << "Current folder:       '" << filesystem::current_path() << "'\n";
        cout << "Actions folder:       '" << FolderActions() << "'\n";
        cout << "Binary data folder:   '" << FolderBinaryPatterns() << "'\n";

        ProcessingInfo jobInfo{
            .file_source = parametersReader.Source(),
            .file_target = parametersReader.Target(),
            .file_actions = parametersReader.Actions(),
            .overwrite = parametersReader.Overwrite()
        };

        retValue = bpatch::ProcessFilesByMask(jobInfo);
    }
    catch (filesystem::filesystem_error const& ex)
    {
        cerr << toconsole("file system ERROR: ") << ex.what() << '\n'
            << "path1: " << ex.path1() << '\n'
            << "path2: " << ex.path2() << '\n'
            << "value: " << ex.code().value() << '\n'
            << "message:  " << ex.code().message() << '\n'
            << "category: " << ex.code().category().name() << '\n';
    }
    catch (range_error& rExc) // must be before runtime_error
    {
        cerr << toconsole("range ERROR: \"")
            << rExc.what()
            << "\""
            << endl;
    }
    catch (runtime_error& rExc)
    {
        cerr << toconsole("runtime ERROR: \"")
            << rExc.what()
            << "\""
            << endl;
    }
    catch (out_of_range& rExc) // must be before logic_error
    {
        cerr << toconsole("out of range ERROR: \"")
            << rExc.what()
            << "\""
            << endl;
    }
    catch (logic_error& rExc)
    {
        cerr << toconsole("logic ERROR: \"")
            << rExc.what()
            << "\""
            << endl;
    }
    catch (exception& rExc)
    {
        cerr << toconsole("ERROR: \"")
            << rExc.what()
            << "\""
            << endl;
    }

    return retValue;
}

}; // namespace bpatch
