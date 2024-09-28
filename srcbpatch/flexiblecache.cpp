#include "stdafx.h"
#include "flexiblecache.h"

namespace bpatch
{
using namespace std;

FlexibleCache::FlexibleCache()
    : rootChunk(new Chunk)
    , currentChunk(&rootChunk)
{
}


bool FlexibleCache::Accumulate(const string_view adata)
{
    unique_ptr<Chunk>& activeChunk = *currentChunk;

    if (activeChunk->accumulated + adata.size() >= bpatch::SZBUFF_FC)
    {
        // calculate sizes to accumulate and to pass further
        const size_t forNewChunk = activeChunk->accumulated + adata.size() - bpatch::SZBUFF_FC;
        const size_t toAccumulate = adata.size() - forNewChunk;

        // accumulate data
        memcpy(activeChunk->data + activeChunk->accumulated, adata.data(), toAccumulate);
        activeChunk->accumulated += toAccumulate;

        const string_view svReminder(adata.data() + toAccumulate, forNewChunk);

        // shift chunk to next
        activeChunk->next.reset(new Chunk);
        currentChunk = &activeChunk->next;
        Accumulate(svReminder);// accumulate reminder
    }
    else
    {
        memcpy(activeChunk->data + activeChunk->accumulated, adata.data(), adata.size());
        activeChunk->accumulated += adata.size();
    }

    return rootChunk->accumulated == bpatch::SZBUFF_FC;
}


bool FlexibleCache::Accumulate(const char toProcess)
{
    unique_ptr<Chunk>& activeChunk = *currentChunk;

    // save byte
    activeChunk->data[activeChunk->accumulated] = toProcess;
    // check overflow
    if (++activeChunk->accumulated >= bpatch::SZBUFF_FC)
    {
        // shift chunk to next
        activeChunk->next.reset(new Chunk);
        currentChunk = &activeChunk->next;
    }

    return rootChunk->accumulated == bpatch::SZBUFF_FC;
}


bool FlexibleCache::Next(unique_ptr<Chunk>& achunk)
{
    if (currentChunk == &rootChunk)
    {
        achunk.swap(rootChunk);
        rootChunk.reset(new Chunk);
        return false;
    }

    const bool reassignCurrentChunk = currentChunk == &rootChunk->next;

    unique_ptr<Chunk> tempChunk;

    tempChunk.swap(rootChunk->next); // root->next  -> nullptr, tempchunk holds remainder chain
    achunk.swap(rootChunk);  // achunk get current root
    rootChunk.swap(tempChunk); // root chunk become root, temp chunk get value from original achunk

    if (reassignCurrentChunk)
    { // next from previous root now is not in chain
      // therefor we need to reassign currentChunk
        currentChunk = &rootChunk;
    }
    return true;
};


};// namespace bpatch

