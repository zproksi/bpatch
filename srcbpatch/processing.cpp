#include "actionscollection.h"
#include "binarylexeme.h"
#include "bpatchfolders.h"
#include "consoleparametersreader.h"
#include "fileprocessing.h"
#include "processing.h"
#include "stdafx.h"
#include "timemeasurer.h"
#include "wildcharacters.h"


namespace bpatch
{
using namespace std;

namespace
{
    struct ProcessingInfo
    {
        std::string_view file_source = "";
        std::string_view file_target = "";
        std::string_view file_actions = "";
        bool overwrite = false;
    };

    struct FileProcessingInfo
    {
        std::unique_ptr<ActionsCollection>& todo;
        std::string& src;
        std::string& dst;
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
/// Setup processing chain for ActionsCollection with Writer
///   Using Reader to read data and send data to Actions collection
/// </summary>
/// <param name="todo">Processing engine - actions collections</param>
/// <param name="pReader">reading of data from file</param>
/// <param name="pWriter">writing data to file</param>
void DoReadReplaceWrite(unique_ptr<ActionsCollection>& todo, Reader* const pReader, Writer* const pWriter)
{
    using namespace std;
    // setup chain to write the data
    todo->SetLastReplacer(StreamReplacer::ReplacerLastInChain(pWriter));

    // hold vector where we are reading data.
    // no new allocations
    vector<char> adata;
    adata.resize(SZBUFF_FC);
    const span dataHolder(adata.data(), SZBUFF_FC);

    do
    {
        auto fullSpan = pReader->ReadData(dataHolder);

        std::ranges::for_each(fullSpan, [&todo](const char c) {todo->DoReplacements(c, false); });

    }while (!pReader->FileReaded());
    todo->DoReplacements('e', true);
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
            DoReadReplaceWrite(jobInfo.todo, &rwProcessing, &rwProcessing);
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
        std::cout << coloredconsole::toconsole("Warning: Target file '") << jobInfo.dst << "' exists. "
            "Use /w instead of /t to overwrite.\n Processing skipped\n";
        jobInfo.written = 0;
        jobInfo.readed = 0;
        return false;
    }

    ReadFileProcessing reader(jobInfo.src.c_str());
    WriteFileProcessing writer(jobInfo.dst.c_str());

    DoReadReplaceWrite(jobInfo.todo, &reader, &writer);
    // we do not resize file here because we have opened/created file only for writing
    jobInfo.written = writer.Written();
    jobInfo.readed = reader.Readed();

    return true;
}


/// <summary>
///  Wild charactes processing level.
/// ActionsCollection will be created here
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
    wildcharacters::LookUp lup;
    lup.RegisterSourceAndDestination(jobInfo.file_source, jobInfo.file_target);
    string srcFilename; // source file name
    string dstFilename; // destination file name

    size_t filesProcessed = 0;
    FileProcessingInfo fileInfo{.todo = todo, .src = srcFilename, .dst = dstFilename, .overwrite = jobInfo.overwrite};
    while (lup.NextFilenamesPair(srcFilename, dstFilename)) // request file names
    {
        std::cout << "Source file:          '" << fileInfo.src << "'\n";
        std::cout << "Target file:          '" << fileInfo.dst << "'\n";
        if (bpatch::ProcessTheFile(fileInfo))
        {
            ++filesProcessed;
        }

        std::cout << "Readed (bytes):       '" << fileInfo.readed << "'\n";
        std::cout << "Written (bytes):      '" << fileInfo.written << "'\n";
        std::cout << "\n";
    };
    std::cout << "Files processed:      '" << filesProcessed << "'\n";

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
        std::cout << parametersReader.Manual();
        return false;
    }

    int retValue = false;
    try
    {
        std::cout << "\n";
        std::cout << "Executable:           '" << argv[0] << "'\n";
        std::cout << "Current folder:       '" << std::filesystem::current_path() << "'\n";
        std::cout << "Actions folder:       '" << FolderActions() << "'\n";
        std::cout << "Binary data folder:   '" << FolderBinaryPatterns() << "'\n";

        ProcessingInfo jobInfo{
            .file_source = parametersReader.Source(),
            .file_target = parametersReader.Target(),
            .file_actions = parametersReader.Actions(),
            .overwrite = parametersReader.Overwrite()
        };

        retValue = bpatch::ProcessFilesByMask(jobInfo);
    }
    catch (std::filesystem::filesystem_error const& ex)
    {
        std::cerr << toconsole("file system ERROR: ") << ex.what() << '\n'
            << "path1: " << ex.path1() << '\n'
            << "path2: " << ex.path2() << '\n'
            << "value: " << ex.code().value() << '\n'
            << "message:  " << ex.code().message() << '\n'
            << "category: " << ex.code().category().name() << '\n';
    }
    catch (std::range_error& rExc) // must be before runtime_error
    {
        std::cerr << toconsole("range ERROR: \"")
            << rExc.what()
            << "\""
            << std::endl;
    }
    catch (std::runtime_error& rExc)
    {
        std::cerr << toconsole("runtime ERROR: \"")
            << rExc.what()
            << "\""
            << std::endl;
    }
    catch (std::out_of_range& rExc) // must be before logic_error
    {
        std::cerr << toconsole("out of range ERROR: \"")
            << rExc.what()
            << "\""
            << std::endl;
    }
    catch (std::logic_error& rExc)
    {
        std::cerr << toconsole("logic ERROR: \"")
            << rExc.what()
            << "\""
            << std::endl;
    }
    catch (std::exception& rExc)
    {
        std::cerr << toconsole("ERROR: \"")
            << rExc.what()
            << "\""
            << std::endl;
    }

    return retValue;
}

}; // namespace bpatch
