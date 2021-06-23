#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include "MemoryConstants.h"

struct searchInfo
{
    uint64_t currFrameIdx;
    uint64_t emptyFrameIdx;
    uint64_t evictedParent;
    uint64_t evictedIdx;
    uint64_t parentAddr;
    uint64_t maxFrameIdx;
    
    int maxWeightFound;
    bool foundEmptyFrame;
};

void clearTable(uint64_t frameIndex)
{
    for (uint64_t i = 0; i < PAGE_SIZE; ++i)
    {
        PMwrite(frameIndex * PAGE_SIZE + i, 0);
    }
}

void VMinitialize()
{
    clearTable(0);
}

/**
 * Try to find an empty frame, update searchInfo appropriately.
 */
void dfs(int depth, uint64_t parentAddr, int oddCount, int evenCount, searchInfo &info)
{
    if (depth == TABLES_DEPTH)
    {
        int weight = oddCount * WEIGHT_ODD + evenCount * WEIGHT_EVEN;
        if(weight > info.maxWeightFound ||
        (weight == info.maxWeightFound && info.currFrameIdx < info.evictedIdx) )
        {
            info.maxWeightFound = weight;
            info.evictedIdx = info.currFrameIdx;
            info.evictedParent = parentAddr;
        }

        return;
    }

    if(info.currFrameIdx > info.maxFrameIdx)
    {
        info.maxFrameIdx = info.currFrameIdx;
    }

    word_t resAddr;
    for (int i = 0; i < PAGE_SIZE; ++i)
    {
        PMread(info.currFrameIdx * PAGE_SIZE + i, &resAddr);

        // current frame is not empty
        if(resAddr != 0)
        {
            uint64_t oldAddr= info.currFrameIdx;
            info.currFrameIdx = resAddr;
            if(oldAddr % 2 == 0)
            {
                dfs(depth + 1,PAGE_SIZE*oldAddr+i, oddCount, evenCount+1, info);
            }
            else
            {
                dfs(depth + 1,PAGE_SIZE*oldAddr+i, oddCount+1, evenCount, info);
            }
            return;
        }
    }
    if(!info.foundEmptyFrame)
    {
        info.foundEmptyFrame = true;
        info.emptyFrameIdx = info.currFrameIdx;
        info.parentAddr = parentAddr;
    }
    //todo: check for correctness
}

/**
 * find an empty frame exists and evict/restore when necessary
 */
uint64_t getEmptyFrame(uint64_t virtualAddress)
{
    searchInfo info = {0, 0, 0,0, false};
    uint64_t page = virtualAddress >> OFFSET_WIDTH;
    if(page % 2 == 0)
    {
        dfs(0,0,0,1, info);
    }
    else{
        dfs(0,0,1,0, info);
    }

    // found frame with empty table
    if(info.foundEmptyFrame && info.emptyFrameIdx != 0)
    {
        PMwrite(info.parentAddr, 0);
        return info.emptyFrameIdx;
    }

    if(info.maxFrameIdx < NUM_FRAMES)
    {
        return info.maxFrameIdx + 1;
    }

    if(!info.foundEmptyFrame)
    {
        PMwrite(info.evictedParent, 0);
        PMevict(info.evictedIdx, page);
        return info.evictedIdx;
    }


    return 0; // todo: change
}


/**
 * get a virtual address and update offsets to contain the offsets
 * for each level in tree
 * @param virtualAddress virtual address to get offsets of
 * @param offsets array of addresses to access each
 */
void getTableOffsets(uint64_t virtualAddress, uint64_t *offsets)
{
    uint64_t mask = (1 << OFFSET_WIDTH) - 1u;

    virtualAddress >>= (unsigned  int) OFFSET_WIDTH;
    for (uint64_t i = 0; i < TABLES_DEPTH; ++i)
    {
        offsets[TABLES_DEPTH - i - 1] = virtualAddress & mask;
        virtualAddress >>= (unsigned int) OFFSET_WIDTH;
    }
}

uint64_t getPhysicalAddress(uint64_t virtualAddress)
{
    word_t idx  = 0;
    word_t currTableIdx = 0;
    uint64_t tableOffsets[TABLES_DEPTH];
    getTableOffsets(virtualAddress, tableOffsets);
    for (unsigned int i = 0; i < TABLES_DEPTH - 1; ++i)
    {
        PMread(currTableIdx * PAGE_SIZE + tableOffsets[i], &idx);
        if (idx == 0)
        {
            idx = getEmptyFrame(virtualAddress);
            clearTable(idx);
            PMwrite(currTableIdx * PAGE_SIZE + tableOffsets[i], idx);
        }
        currTableIdx = idx;
    }

    word_t res;
    PMread(currTableIdx * PAGE_SIZE + tableOffsets[TABLES_DEPTH - 1], &res);

    if(res == 0)
    {
        res = getEmptyFrame(virtualAddress);
        PMrestore(res, virtualAddress >> OFFSET_WIDTH);
        PMwrite(currTableIdx * PAGE_SIZE + tableOffsets[TABLES_DEPTH - 1], res);
    }

    word_t offset = virtualAddress & ((1 << OFFSET_WIDTH) -1);

    return res * PAGE_SIZE + offset;
}

int VMread(uint64_t virtualAddress, word_t *value)
{
    if (virtualAddress > VIRTUAL_MEMORY_SIZE)
    {
        return 0;
    }
    uint64_t physicalAddress = getPhysicalAddress(virtualAddress);
    PMread(physicalAddress, value);
    return 1;
}


int VMwrite(uint64_t virtualAddress, word_t value)
{
    if (virtualAddress > VIRTUAL_MEMORY_SIZE)
    {
        return 0;
    }
    uint64_t physicalAddress = getPhysicalAddress(virtualAddress);
    PMwrite(physicalAddress, value);

    //deleteme
    word_t val;
    PMread(physicalAddress, &val);
    return 1;
}
