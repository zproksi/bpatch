#pragma once
#include <filesystem>
#include <limits>
#include <span>
#include <vector>
#include "flexiblecache.h"

namespace bpatch
{
//------------------------------------------------------
/// <summary>
///  interface for source of the data
/// </summary>
class Reader
{
    Reader(const Reader&) = delete;
    Reader& operator =(const Reader&) = delete;
    Reader(Reader&&) = delete;
    Reader& operator =(Reader&&) = delete;
public:
    Reader() = default;
    virtual ~Reader() = default;


    /// <summary>
    ///   reads data to span. span should contain maximum ampount of data to read
    /// </summary>
    /// <param name="place">place where readed data to hold. and maximum data to read</param>
    /// <returns>the span but with data amount readed</returns>
    virtual std::span<char> ReadData(const std::span<char> place) = 0;


    /// <summary>
    ///   Check if read everything
    /// </summary>
    /// <returns> true if we read all the data</returns>
    virtual bool FileReaded()const noexcept = 0;


    /// <summary>
    ///   how much data we have readed already
    /// </summary>
    /// <returns>amount of data already readed from file</returns>
    virtual size_t Readed() const noexcept = 0;
};


//------------------------------------------------------
/// <summary>
///  interface for writing of the data
/// </summary>
class Writer
{
    Writer(const Writer&)= delete;
    Writer& operator =(const Writer&) = delete;
    Writer(Writer&&) = delete;
    Writer& operator =(Writer&&) = delete;
public:
    Writer() = default;
    virtual ~Writer() = default;


    /// <summary>
    ///   Write character to somewhere unless aEod is true
    /// </summary>
    /// <param name="toProcess">character to write</param>
    /// <param name="aEod">true if it is end of data and we should not write</param>
    /// <returns>Actually written data. Could be 0 if we just accumulated character</returns>
    virtual size_t WriteCharacter(const char toProcess, const bool aEod) = 0;


    /// <summary>
    ///   how much data we have written already
    /// </summary>
    /// <returns>amount of data already written to file</returns>
    virtual size_t Written() const noexcept = 0;
};


//------------------------------------------------------

/// <summary>
///  Class holds basic file stream.
///      closes it in the destructor
/// </summary>
class FileProcessing
{
    FileProcessing(const FileProcessing&) = delete;
    FileProcessing& operator=(const FileProcessing&) = delete;
    FileProcessing(FileProcessing&&) = delete;
    FileProcessing& operator=(FileProcessing&&) = delete;
public:

    /// <summary>
    ///   opens file with requested mode
    /// </summary>
    /// <param name="fname">file name to open</param>
    /// <param name="mode">mode of file to open. See std::fopen documentation</param>
    FileProcessing(const char* fname, const char* mode);

    /// <summary>
    ///  closes file here
    /// </summary>
    ~FileProcessing();
protected:
    /// <summary>
    ///  our stream for read or write
    /// </summary>
    std::FILE* stream_ = nullptr;

private:
    /// <summary>
    ///  bufferization of io
    /// </summary>
    std::vector<char> buff_;
};


//------------------------------------------------------
/// <summary>
/// Read data from zero position.
/// </summary>
class ReadFileProcessing : virtual public FileProcessing, public Reader
{
public:
    /// <summary>
    ///   open file for reading only
    /// </summary>
    /// <param name="fname">file name to open</param>
    /// <param name="mode"> mode of file - rb</param>
    ReadFileProcessing(const char* fname, const char* mode = "rb");

    /// <summary>
    ///   Read Data from file and put it into span
    ///       total length of data in span can be 0
    ///       if end of file reached
    /// </summary>
    /// <param name="place">place where readed data to hold. and maximum data to read</param>
    /// <returns>the span but with data amount readed</returns>
    std::span<char> ReadData(const std::span<char> place) override;

    /// <summary>
    ///   Check if read everything from file
    /// </summary>
    /// <returns> true if we read all the data from file</returns>
    bool FileReaded()const noexcept override {return eof_;}


    /// <summary>
    ///   how much data we have readed already
    /// </summary>
    /// <returns>amount of data already readed from file</returns>
    size_t Readed() const noexcept override {return readedAmount_;};
protected:
    bool eof_ = false; // if we reached end of file during reading
    size_t readedAmount_ = 0; // how many bytes we have readed
};


//------------------------------------------------------
class FlexibleCache;
/// <summary>
/// Write data from zero position.
/// </summary>
class WriteFileProcessing : virtual public FileProcessing, public Writer
{
public:
    /// <summary>
    ///   Creates/overwrites file for writing
    /// </summary>
    /// <param name="fname">file name to write to</param>
    /// <param name="mode">writing mode - wb is default</param>
    WriteFileProcessing(const char* fname, const char* mode = "wb");

    size_t WriteCharacter(const char toProcess, const bool aEod) override;

    size_t Written() const noexcept override; // the only way to get writeAt_

protected:
    size_t WriteAndThrowIfFail(const std::string_view& sv); // here writeAt_ could grow

    /// <summary>
    ///   write either full chunks only or everything if aEod was achieved
    /// </summary>
    /// <param name="aEod">end of data is here - write everything</param>
    /// <returns>number of written bytes</returns>
    size_t WriteEverythingOrFullChunks(const bool aEod);

    // we will write to the file only by big chunks or if the data ends
    // data will be accumulated here
    std::unique_ptr<FlexibleCache> cache_;

private:
    size_t writeAt_ = 0; // now we are writing at
};


///------------------------------------------------------
/// <summary>
/// Read data from zero position.
///   Write data from zero position but not further than you have readed
/// </summary>
class ReadWriteFileProcessing final : virtual public ReadFileProcessing, virtual public WriteFileProcessing
{
public:

    /// <summary>
    ///   opens file for reading/writing
    /// </summary>
    /// <param name="fname">file name to read from/write to</param>
    /// <param name="mode">writing mode - r+b is default.
    ///   read from start</param>
    ReadWriteFileProcessing(const char* fname, const char* mode = "r+b");


    /// <summary>
    /// save readAt_ and continue reading from readAt_ always.
    /// set readAt_ to maximum size_t when file has been readed
    /// </summary>
    /// <param name="place">place where readed data to hold. and maximum data to read</param>
    /// <returns>the span but with data amount readed</returns>
    ///   throws in case of error</returns>
    std::span<char> ReadData(const std::span<char> place) override;


    /// <summary>
    /// save writeAt_ and continue writing from writeAt_ always.
    /// write not later than readAt_.
    /// </summary>
    /// <param name="toProcess">character to add to cache</param>
    /// <param name="aEod">sign that no more data in the current session
    ///   to write everything what was cached</param>
    /// <returns>how may bytes were written into the  file (not chached)</returns>
    size_t WriteCharacter(const char toProcess, const bool aEod) override;


protected:
    /// <summary>
    ///   Set position in file for reading or for writing
    /// </summary>
    /// <param name="bReading"> true if we need to read</param>
    /// <returns>throws if fail</returns>
    void SeekSet(const bool bReading);

};


/// <summary>
///   Reads full file to provided vector
/// Initially, just check if the fname is file and has size.
/// opens the file if found it. If file cannot be found merges fname to
///  provided additionalPath and search again
/// </summary>
/// <param name="readTo">vector to read data to</param>
/// <param name="fname">the filename of the file to open</param>
/// <param name="additionalPath">path where search the file if it was not open in the current folder</param>
/// <returns>true if file exists and data readed, even if the file has 0 size</returns>
bool ReadFullFile(std::vector<char>& readTo, const char* const fname, const std::filesystem::path& additionalPath);

};// namespace bpatch
