#pragma once
#include <memory>
#include <string_view>

namespace bpatch
{
/// <summary>
///  this constant will be used to read data in chunks of such size
/// </summary>
constexpr static const std::size_t SZBUFF_FC = 1024 * 1024;


/// <summary>
///    accumulating data in dynamic memory in chunks. 
/// </summary>
class FlexibleCache
{
public:
    /// <summary>
    ///   accumulating data in such chunks
    /// </summary>
    struct Chunk
    {
        std::unique_ptr<Chunk> next;
        char data[bpatch::SZBUFF_FC];
        size_t accumulated = 0;
    };

public:
    FlexibleCache();

    /// <summary>
    ///    accumulate data inside
    /// </summary>
    /// <param name="data"> data to accumulate</param>
    /// <returns>true if root chunk completely filled with data</returns>
    bool Accumulate(const std::string_view& adata);


    /// <summary>
    ///    accumulate character inside
    /// </summary>
    /// <param name="toProcess"> character to accumulate</param>
    /// <returns>true if root chunk completely filled with data</returns>
    bool Accumulate(const char toProcess);


    /// <summary>
    ///    Translates ownership of the root chunk to the requestor
    /// </summary>
    /// <param name="achunk"> root chunk will be returned here</param>
    /// <returns>true in case if the data in the data chain remains</returns>
    bool Next(std::unique_ptr<Chunk>& achunk);

    /// <summary>
    ///    returns true if the very first chunk is full with data
    /// </summary>
    /// <returns>true if the very first chunk is full with data</returns>
    bool RootChunkFull()const
    {
        return rootChunk->accumulated == bpatch::SZBUFF_FC;
    }


protected:

    std::unique_ptr<Chunk> rootChunk; // very first chunk
    std::unique_ptr<Chunk>* currentChunk; // chunk where current accumulate process happens
};


};// namespace bpatch

