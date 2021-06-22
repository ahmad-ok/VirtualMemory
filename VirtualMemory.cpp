#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include "MemoryConstants.h"

struct searchInfo
{
    uint64_t currAddr;
    uint64_t emptyAddr;
    uint64_t parentAddr;
    int parentOffset;
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
void dfs(int depth, int parentOffset, uint64_t parentAddr, searchInfo &info)
{
    if (depth == TABLES_DEPTH)
    {
        return;
    }
    for (int i = 0; i < PAGE_SIZE; ++i)
    {
        word_t resAddr = 0;
        PMread(info.currAddr * PAGE_SIZE + i,&resAddr);

        // current frame is not empty
        if(resAddr != 0)
        {
            uint64_t oldAddr= info.currAddr;
            info.currAddr = resAddr;
            dfs(depth + 1, i, oldAddr, info);
        }
    }
    if(!info.foundEmptyFrame)
    {
        info.foundEmptyFrame = true;
        info.parentOffset = parentOffset;
        info.parentAddr = parentAddr;
        info.emptyAddr = info.currAddr;
    }
    //todo: check for correctness
}

/**
 * find an empty frame exists and evict/restore when necessary
 */
uint64_t getEmptyFrame()
{
    searchInfo info = {0, 0, 0, 0, false};
    dfs(0,0,0, info);



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
    uint64_t mask = 1u << (OFFSET_WIDTH - 1u);

    for (uint64_t i = 0; i < TABLES_DEPTH; ++i)
    {
        offsets[TABLES_DEPTH - i - 1] = virtualAddress & mask;
        virtualAddress >>= (unsigned int) OFFSET_WIDTH;
    }

}

uint64_t getPhysicalAddress(uint64_t virtualAddress)
{
    word_t resAddr = 0;
    uint64_t tableOffsets[TABLES_DEPTH];
    getTableOffsets(virtualAddress, tableOffsets);
    for (unsigned int i = 0; i < TABLES_DEPTH - 1; ++i)
    {
        PMread(resAddr * PAGE_SIZE + tableOffsets[i], &resAddr);
        if (resAddr == 0)
        {
            uint64_t unusedFrame = getEmptyFrame();
            clearTable(unusedFrame);
            PMwrite(resAddr * PAGE_SIZE + tableOffsets[i], unusedFrame);
        }
    }

    return 0; //todo: change
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
