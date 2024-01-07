#include "stdafx.h"
#include "actionscollection.h"
#include "binarylexeme.h"
#include "consoleparametersreader.h"
#include "fileprocessing.h"
#include "processing.h"
#include "timemeasurer.h"
#include "bpatchfolders.h"


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
        size_t readed = 0;
        size_t written = 0;
        bool overwrite = false;
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


void DoReadReplaceWrite(unique_ptr<ActionsCollection>& todo, Reader* const pReader, Writer* const pWriter)
{
    using namespace std;
    // setup chain to write the data
    auto lastReplacer = StreamReplacer::ReplacerLastInChain(pWriter);
    todo->SetNextReplacer(lastReplacer.get());

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


bool ProcessTheFile(ProcessingInfo& jobInfo)
{
    using namespace std;
    /// --------------------------------------------------------
    /// load Actions and initialize processing class
    /// Json parsing is inside
    unique_ptr<ActionsCollection> todo = CreateActionsFile(jobInfo.file_actions);

    /// --------------------------------------------------------
    /// if source and target file are the same
    /// -- processing inplace --
    /// 
    if (jobInfo.file_source.data() == jobInfo.file_target.data()
        ||
        ranges::equal(jobInfo.file_source, jobInfo.file_target)
        )
    {
        {
            ReadWriteFileProcessing rwProcessing(jobInfo.file_source.data());
            DoReadReplaceWrite(todo, &rwProcessing, &rwProcessing);
            jobInfo.written = rwProcessing.Written();
            jobInfo.readed = rwProcessing.Readed();
        } // close file

        // set file size
        // because we can write less than read
        filesystem::resize_file(jobInfo.file_source, jobInfo.written);
        return true; // inplace processing has been done
    }


    /// -------------------------------------------------------
    /// source and target are different files
    /// -- processing reading and writing in different files --
    /// 
    error_code ec;
    if (!jobInfo.overwrite &&
        filesystem::exists(jobInfo.file_target, ec))
    { // check override possibility
        stringstream ss;
        ss << "Target file '" << jobInfo.file_target << "' exists. "
            "Use /w instead of /t to ovewrite.";
        throw runtime_error(ss.str().c_str());
    }

    WriteFileProcessing writer(jobInfo.file_target.data());
    ReadFileProcessing reader(jobInfo.file_source.data());

    DoReadReplaceWrite(todo, &reader, &writer);
    // we do not resize file here because we have opened/created file only for writing
    jobInfo.written = writer.Written();
    jobInfo.readed = reader.Readed();

    return true;
}

bpatch::ConsoleParametersReader parametersReader;



bool Processing(int argc, char* argv[])
{
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
        std::cout << "Executable:         '" << argv[0] << "'\n";
        std::cout << "Current folder:     '" << std::filesystem::current_path() << "'\n";
        std::cout << "Actions folder:     '" << FolderActions() << "'\n";
        std::cout << "Binary data folder: '" << FolderBinaryPatterns() << "'\n";

        ProcessingInfo jobInfo{
            .file_source = parametersReader.Source(),
            .file_target = parametersReader.Target(),
            .file_actions = parametersReader.Actions(),
            .overwrite = parametersReader.Overwrite()
        };

        std::cout << "Source file:        '" << jobInfo.file_source << "'\n";
        std::cout << "Actions file:       '" << jobInfo.file_actions << "'\n";
        std::cout << "Target file:        '" << jobInfo.file_target << "'\n";

        retValue = bpatch::ProcessTheFile(jobInfo);

        std::cout << "Readed (bytes):     '" << jobInfo.readed << "'\n";
        std::cout << "Written (bytes):    '" << jobInfo.written << "'\n";
        std::cout << "\n";
    }
    catch (std::filesystem::filesystem_error const& ex)
    {
        std::cerr << "file system ERROR: " << ex.what() << '\n'
            << "path1: " << ex.path1() << '\n'
            << "path2: " << ex.path2() << '\n'
            << "value: " << ex.code().value() << '\n'
            << "message:  " << ex.code().message() << '\n'
            << "category: " << ex.code().category().name() << '\n';
    }
    catch (std::range_error& rExc) // must be before runtime_error
    {
        std::cerr << "range ERROR: \""
            << rExc.what()
            << "\""
            << std::endl;
    }
    catch (std::runtime_error& rExc)
    {
        std::cerr << "runtime ERROR: \""
            << rExc.what()
            << "\""
            << std::endl;
    }
    catch (std::out_of_range& rExc) // must be before logic_error
    {
        std::cerr << "out of range ERROR: \""
            << rExc.what()
            << "\""
            << std::endl;
    }
    catch (std::logic_error& rExc)
    {
        std::cerr << "logic ERROR: \""
            << rExc.what()
            << "\""
            << std::endl;
    }
    catch (std::exception& rExc)
    {
        std::cerr << "ERROR: \""
            << rExc.what()
            << "\""
            << std::endl;
    }

    return retValue;
}

}; // namespace bpatch
