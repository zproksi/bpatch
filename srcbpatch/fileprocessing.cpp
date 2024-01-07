#include "stdafx.h"
#include "fileprocessing.h"

namespace
{
    const char* const fio_errors[] =
    {
        "Failed to open file." // 0
        , "Failed to write a file." // 1
        , "Failed to read a file." // 2
        , "Failed to prepare a file for IO operation." // 3

    };

};



namespace bpatch
{
using namespace std;
using namespace std::filesystem;


FileProcessing::FileProcessing(const char* fname, const char* mode)
{
#pragma warning(disable: 4996) // I would like to use fopen instead of fopen_s, because
                               // 1. I would like to write fast code
                               // 2. MSVS compiler does not respect C++ standard and __STDC_LIB_EXT1__ define
    if (stream_ = fopen(fname, mode); nullptr == stream_)
#pragma warning(default: 4996)
    {
        throw filesystem_error(fio_errors[0], filesystem::path(fname), error_code());
    }
    buff_.resize(SZBUFF_FC);
    if (setvbuf(stream_, buff_.data(), _IOFBF, SZBUFF_FC) != 0)
    {
        perror("Warning: buff_ering for file processing has not initialized.");
    }
}

FileProcessing::~FileProcessing()
{
    if (stream_)
    {
        fflush(stream_);
        fclose(stream_);
    }
    stream_ = nullptr;
}


ReadFileProcessing::ReadFileProcessing(const char* fname, const char* mode)
    : FileProcessing(fname, mode)
{
}

span<char> ReadFileProcessing::ReadData(const span<char> place)
{
    const size_t readed = fread(place.data(), sizeof(*place.data()), place.size(), stream_);
    readedAmount_ += readed;

    eof_ = feof(stream_) != 0;
    if (readed < place.size() && !eof_)
    {
        throw filesystem_error(fio_errors[2], error_code());
    }

    return span(place.data(), readed);
}
//------------------------------------------------------


WriteFileProcessing::WriteFileProcessing(const char* fname, const char* mode)
    : FileProcessing(fname, mode)
    , cache_(new FlexibleCache)
{
}


size_t WriteFileProcessing::WriteCharacter(const char toProcess, const bool aEod)
{
    if (!aEod)
    {
        cache_->Accumulate(toProcess);
    }

    return WriteEverythingOrFullChunks(aEod);
}

size_t WriteFileProcessing::Written() const noexcept
{
    return writeAt_;
}

size_t WriteFileProcessing::WriteAndThrowIfFail(const string_view& sv)
{
    const size_t written = fwrite(sv.data(), sizeof(sv.data()[0]), sv.size(), stream_);
    if (written != sv.size())
    {
        throw filesystem_error(fio_errors[1], error_code());
    };
    writeAt_ += written;
    return written;
}

size_t WriteFileProcessing::WriteEverythingOrFullChunks(const bool aEod)
{
    size_t written = 0;
    while (aEod || cache_->RootChunkFull())
    {
        unique_ptr<FlexibleCache::Chunk> chunk;
        const bool dataRemain = cache_->Next(chunk);
        written += WriteAndThrowIfFail(string_view(chunk->data, chunk->accumulated));

        if (!dataRemain) // no more data: leave the loop
            break;

    }
    return written;
}


ReadWriteFileProcessing::ReadWriteFileProcessing(const char* fname, const char* mode)
    : FileProcessing(fname, mode)
    , ReadFileProcessing(fname, mode)
    , WriteFileProcessing(fname, mode)
{

}

span<char> ReadWriteFileProcessing::ReadData(const span<char> place)
{
    SeekSet(true);// reading
    return ReadFileProcessing::ReadData(place);
}


size_t ReadWriteFileProcessing::WriteCharacter(const char toProcess, const bool aEod)
{
    SeekSet(false); // writing

    if (aEod)
    {
        // end of data - need to write everything
        return WriteFileProcessing::WriteEverythingOrFullChunks(aEod);
    }

    bool chunkAccumulated = cache_->Accumulate(toProcess);
    if (!chunkAccumulated)
    {
        return 0;
    }

    size_t writtenRet = 0;

    size_t maxToWrite = FileReaded() ? SZBUFF_FC : readedAmount_ - Written();
    while (maxToWrite >= SZBUFF_FC && chunkAccumulated)
    {
        unique_ptr<FlexibleCache::Chunk> chunk;
        cache_->Next(chunk);

        writtenRet += WriteAndThrowIfFail(string_view(chunk->data, chunk->accumulated));

        // get status of next chunk if it is accumulted
        chunkAccumulated = cache_->RootChunkFull();
        maxToWrite = FileReaded() ? SZBUFF_FC : readedAmount_ - Written();
    };
    return writtenRet;

}


void ReadWriteFileProcessing::SeekSet(const bool bReading)
{
    if (const int ret = fseek(stream_, 
        static_cast<long>(bReading ? readedAmount_ : Written()),
        SEEK_SET); ret != 0)
    {
        throw filesystem_error(fio_errors[3], error_code());
    };
}


bool ReadFullFile(std::vector<char>& readTo, const char* const fname, const std::filesystem::path& additionalPath)
{
    namespace fs = std::filesystem;
    using namespace std;

    fs::path aName(fname);
    error_code ec;
    fs::file_status s = fs::status(aName, ec);
    if (!fs::exists(s) || fs::is_directory(s))
    { // search file, not folder
        aName = additionalPath / aName;

        s = fs::status(aName, ec);
        if (!fs::exists(s) || fs::is_directory(s)) // search file, not folder in the additional path
        {
            return false; // file not found
        }
    }

    // get file size returns error
    const size_t szFile = static_cast<size_t>(filesystem::file_size(aName, ec));
    if (ec)
    { // error is not empty
        std::cout << "Warning: Detection of the size of a file '" << aName << "' returned error '"
            << ec.message() << "'" << endl;
    }

    // resize vector to accumulate data
    readTo.resize(szFile);
    ReadFileProcessing readFile(aName.string().c_str());
    // read the data
    const auto readedSpan = readFile.ReadData(span(readTo.data(), szFile));
    return readedSpan.size() == szFile;
}


};// namespace bpatch