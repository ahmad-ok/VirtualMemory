#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include "MemoryConstants.h"

void clearTable(uint64_t frameIndex) {
    for (uint64_t i = 0; i < PAGE_SIZE; ++i) {
        PMwrite(frameIndex * PAGE_SIZE + i, 0);
    }
}

void VMinitialize() {
    clearTable(0);
}

uint64_t getEmptyFrame()
{
    //todo: implement
    return 0;
}

/**
 * get a virtual address and update offsets to contain the offsets
 * for each level in tree
 * @param virtualAddress virtual address to get offsets of
 * @param offsets array of addresses to access each
 */
void getTableOffsets(uint64_t virtualAddress, uint64_t* offsets)
{
    uint64_t mask = 1u << (OFFSET_WIDTH - 1u);

    for (uint64_t i = 0; i < TABLES_DEPTH; ++i)
    {
        offsets[TABLES_DEPTH-i-1] = virtualAddress & mask;
        virtualAddress >>= (unsigned int)OFFSET_WIDTH;
    }

}
uint64_t getPhysicalAddress(uint64_t virtualAddress)
{
    word_t resAddr = 0;
    uint64_t tableOffsets[TABLES_DEPTH];
    getTableOffsets(virtualAddress, tableOffsets);
    for (unsigned int i = 0; i < TABLES_DEPTH - 1; ++i)
    {
        uint64_t currPhysicalAddr = resAddr * PAGE_SIZE + tableOffsets[i];
        PMread(currPhysicalAddr,&resAddr);
        if(resAddr == 0)
        {
             uint64_t unusedFrame = getEmptyFrame();
             clearTable(unusedFrame);
             PMwrite(currPhysicalAddr, unusedFrame);
        }
    }

    return 0;
}

int VMread(uint64_t virtualAddress, word_t* value)
{
    if(virtualAddress > VIRTUAL_MEMORY_SIZE)
    {
        return 0;
    }
    uint64_t physicalAddress = getPhysicalAddress(virtualAddress);
    PMread(physicalAddress, value);
    return 1;
}


int VMwrite(uint64_t virtualAddress, word_t value)
{
    if(virtualAddress > VIRTUAL_MEMORY_SIZE)
    {
        return 0;
    }
    uint64_t physicalAddress = getPhysicalAddress(virtualAddress);
    PMwrite(physicalAddress, value);
    return 1;
}
