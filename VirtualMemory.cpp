#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include "MemoryConstants.h"

struct searchInfo
{
   uint64_t emptyFrameIdx;
   uint64_t parentAddr;
   uint64_t maxFrameFound;
   int maxWeightFound;
   uint64_t maxWeightFrameIdx;
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
void dfs(searchInfo& info, uint64_t currFrameIdx, uint64_t prevFrame, uint64_t parentAddr, int currWeight, int currDepth)
{

    if(currFrameIdx > info.maxFrameFound)
    {
        info.maxFrameFound = currFrameIdx;
    }

    if(currDepth == TABLES_DEPTH)
    {

        if(currWeight > info.maxWeightFound)
        {
            info.maxWeightFound = currWeight; //todo fix it
            info.maxWeightFrameIdx = currFrameIdx;
            info.parentAddr = parentAddr;
        }

        return;
    }



    bool foundEmpty = true;
    for(int i = 0 ; i < PAGE_SIZE; ++i)
    {
        word_t res;
        PMread(currFrameIdx * PAGE_SIZE + i, &res);
        if(res != 0)
        {
            foundEmpty = false;
            int weight = (currFrameIdx % 2 == 0)? WEIGHT_EVEN : WEIGHT_ODD;
            dfs(info, res, prevFrame, currFrameIdx * PAGE_SIZE + i, currWeight + weight, currDepth + 1);
        }
    }

    if(foundEmpty && currFrameIdx != prevFrame)
    {
        info.emptyFrameIdx = currFrameIdx;
        info.parentAddr = parentAddr;
    }

}

/**
 * find an empty frame exists and evict/restore when necessary
 */
uint64_t getEmptyFrame(uint64_t evictedFrameIdx, uint64_t prevFrame)
{
    searchInfo info = {0, 0, 0, 0};
    dfs(info, 0, prevFrame, 0, 0, 0);
    // case 1 : found empty frame
    if(info.emptyFrameIdx != 0)
    {
        PMwrite(info.parentAddr, 0);
        return info.emptyFrameIdx;
    }
    // case 2 : max + 1 < frame
    if(info.maxFrameFound + 1 < NUM_FRAMES)
    {
        return info.maxFrameFound + 1;
    }
    // case 3 : evict leaf table
    PMwrite(info.parentAddr, 0);
    PMevict(info.maxWeightFrameIdx, evictedFrameIdx);
    return info.maxWeightFrameIdx;
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
    uint64_t offset = ((1 << OFFSET_WIDTH) - 1u) & virtualAddress;
    uint64_t lastfound = 0;
    int flag = 1;
    for (unsigned int i = 0; i < TABLES_DEPTH; ++i)
    {
        PMread(currTableIdx * PAGE_SIZE + tableOffsets[i], &idx);
        if (idx == 0)
        {
            // todo Check if This Part is working
            idx = getEmptyFrame(virtualAddress >> OFFSET_WIDTH, lastfound);
            lastfound = idx;
            clearTable(idx);
            PMwrite(currTableIdx * PAGE_SIZE + tableOffsets[i], idx);
            flag = 0;
        }
        currTableIdx = idx;

    }


    if (flag == 0)
    {
        uint64_t restoredPageIndex = virtualAddress >> OFFSET_WIDTH;
        PMrestore(currTableIdx, restoredPageIndex);
    }
    return currTableIdx * PAGE_SIZE + offset;


//    word_t res;
//    PMread(currTableIdx * PAGE_SIZE + tableOffsets[TABLES_DEPTH - 1], &res);
//
//    if(res == 0)
//    {
//        PMrestore(res, virtualAddress >> OFFSET_WIDTH);
//    }
//    return currTableIdx * PAGE_SIZE + offset;

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

//    //deleteme
//    word_t val;
//    PMread(physicalAddress, &val);
    return 1;
}