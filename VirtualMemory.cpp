#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include "MemoryConstants.h"

struct searchInfo
{
    uint64_t currFrameIdx;
    uint64_t emptyFrameIdx;
    uint64_t evictedParent;
    uint64_t evictedIdx;
    int parentAddr;
    uint64_t maxFrameIdx;
    int maxWeightFound;
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
void dfs(int depth, uint64_t parentAddr, int oddCount, int evenCount,uint64_t lastFoundIdx, searchInfo &info)
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
    bool foundEmpty = true;
    for (int i = 0; i < PAGE_SIZE; ++i)
    {
        PMread(info.currFrameIdx * PAGE_SIZE + i, &resAddr);

        // current frame is not empty
        if(resAddr != 0)
        {
            foundEmpty = false;
            uint64_t oldAddr= info.currFrameIdx;
            info.currFrameIdx = resAddr;
            if(oldAddr % 2 == 0)
            {
                dfs(depth + 1,PAGE_SIZE*oldAddr+i, oddCount, evenCount+1,lastFoundIdx, info);
            }
            else
            {
                dfs(depth + 1,PAGE_SIZE*oldAddr+i, oddCount+1, evenCount, lastFoundIdx,info);
            }
        }
    }
    if(foundEmpty && info.emptyFrameIdx != lastFoundIdx)
    {
        info.emptyFrameIdx = info.currFrameIdx;
        info.parentAddr = parentAddr;
    }
    //todo: check for correctness
}

/**
 * find an empty frame exists and evict/restore when necessary
 */
uint64_t getEmptyFrame(uint64_t virtualAddress, uint64_t lastFoundIdx)
{
    searchInfo info = {0, 0, 0,0, false};
    info.evictedParent = -1;
    info.evictedIdx = 0;
    uint64_t page = virtualAddress >> OFFSET_WIDTH;
    if(page % 2 == 0)
    {
        dfs(0,-1,0,1, lastFoundIdx,info);
    }
    else{
        dfs(0,-1,1,0, lastFoundIdx, info);
    }

    // found frame with empty table
    if(info.emptyFrameIdx != 0)
    {
        if(info.parentAddr != -1)
        {
            PMwrite(info.parentAddr, 0);
        }
    }
    if(info.maxFrameIdx + 1 < NUM_FRAMES)
    {
        return info.maxFrameIdx + 1;
    }

    // otherwise we have to evict
    if(info.evictedIdx != 0 and info.evictedParent != -1)
    {
        PMwrite(info.evictedParent, 0);
        PMevict(info.evictedIdx, page);
        return info.evictedIdx;
    }
    return 0;

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
    uint64_t lastFoundIdx = 0;
    int flag = 0;
    for (unsigned int i = 0; i < TABLES_DEPTH - 1; ++i)
    {
        PMread(currTableIdx * PAGE_SIZE + tableOffsets[i], &idx);
        if (idx == 0)
        {
            idx = getEmptyFrame(virtualAddress, lastFoundIdx);
            lastFoundIdx = idx;
            clearTable(idx);
            flag = 1;
            PMwrite(currTableIdx * PAGE_SIZE + tableOffsets[i], idx);
        }
        currTableIdx = idx;
    }

    word_t res;
    PMread(currTableIdx * PAGE_SIZE + tableOffsets[TABLES_DEPTH - 1], &res);
    if(res == 0)
    {
        res = getEmptyFrame(virtualAddress, lastFoundIdx);
        PMrestore(res, virtualAddress >> OFFSET_WIDTH);
        PMwrite(currTableIdx * PAGE_SIZE + tableOffsets[TABLES_DEPTH - 1], res);
    }

    word_t offset = virtualAddress & (((uint64_t)1 << OFFSET_WIDTH) -1u);

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
    return 1;
}
