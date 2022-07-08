#include "VirtualMemory.h"
#include "PhysicalMemory.h"

/* ============================================== FUNCTIONAL-MACROS ================================================= */
#define PAGE_WIDTH_MASK ((1UL << OFFSET_WIDTH) - 1)

#define MIN(a,b) a < b ? a : b;
#define ABS(a,b) a < b ? b-a : a-b;
/**
 * return table * PAGE_SIZE + i
 */
#define TABLE_AT_I(table,i) ((uint64_t) (table << OFFSET_WIDTH) + i)
/**
 * return the hist on the i'th table of this virtual address (the value of the i'th chunk of addr)
 */
#define Ith_TABLE_VAL(i,addr) ((addr >> ((TABLES_DEPTH - i) * OFFSET_WIDTH)) & PAGE_WIDTH_MASK)

/* ==================================================== HELPERS ===================================================== */
/**
 * Init all words in a frame to 0
 * @param frameIndex number of frame to clear
 */
void clearTable(uint64_t frameIndex) {
    for (uint64_t i = 0; i < PAGE_SIZE; ++i) {
        PMwrite(frameIndex * PAGE_SIZE + i, 0);
    }
}

/**
 * Depth first search on the tables tree, in order to find the next valid frame.
 * This frame might be an empty table, a non-alloced one or a designated for eviction.
 * @param pageToLoad number of page we try to load on the RAM
 * @param forbiddenFrame number of frame, which we got from the previous call for findNextEmptyFrame, so we cannot pick it.
 * @param maxFrameAlloced number of the max alloced frame we had found yet
 * @param maxCyclicDist to contain the max cyclic distance from pageToLoad of a page in the tree
 * @param pageToEvict to contain this max cyclic distance page
 * @param frameToEvict to contain the frame of pageToEvict
 * @param parentOfEvicted to contain the parent of pageToEvict address
 * @param curDepth the current depth on the tree search
 * @param curFrame the current frame/table (node in the tree) we look on
 * @param curPath the current path a page was traversing the tree
 * @param parentPhysicalPointer the parent of the current frame
 * @return frame number > 0 if an empty frame was found, 0 else
 */
uint64_t dfs(const uint64_t& pageToLoad, const uint64_t& forbiddenFrame, uint64_t &maxFrameAllocated,
             uint64_t &maxCyclicDist, uint64_t &pageToEvict, uint64_t &frameToEvict , uint64_t &parentOfEvicted,
             int curDepth,  uint64_t curFrame, uint64_t curPath, uint64_t parentPhysicalPointer) {

    if (curFrame > maxFrameAllocated) maxFrameAllocated = curFrame; // updating the maximum *alloced* frame index

    // base case2: this frame was just cleared, so we cannot use it
    if (forbiddenFrame == curFrame) return 0;

    // base case3: curFrame is a physical address of a word, so only check if that curPage has the maxCycleDist for getting evicted
    if (curDepth == TABLES_DEPTH) {
        uint64_t pagesCycleDif = ABS(pageToLoad, curPath);
        uint64_t currCyclicDist = MIN(NUM_PAGES - pagesCycleDif, pagesCycleDif);

        if (currCyclicDist > maxCyclicDist) {
            maxCyclicDist = currCyclicDist;
            pageToEvict = curPath;
            frameToEvict = curFrame;
            parentOfEvicted = parentPhysicalPointer;
        }
        return 0;
    }

    bool frameIsEmpty = true;
    for (uint64_t i = 0; i < PAGE_SIZE; ++i) {

        word_t nextFrame = 0;
        PMread(TABLE_AT_I(curFrame, i), &nextFrame);
        /* recursion */
        if (nextFrame != 0) {
          frameIsEmpty = false;
          uint64_t frame = dfs(pageToLoad, forbiddenFrame, maxFrameAllocated,
                               maxCyclicDist, pageToEvict, frameToEvict, parentOfEvicted,
                               curDepth+1, nextFrame, TABLE_AT_I(curPath, i),TABLE_AT_I(curFrame, i));

          // one of nextFrame descendants (sub-tables) is an empty frame, so we can return it
          if (frame != 0) return frame;
        }
    }

    // base case1: curFrame is an empty one
    if (frameIsEmpty) {
        PMwrite(parentPhysicalPointer, 0);
        return curFrame;
    }

    return 0;
}

/**
 * Finding a free frame to contain the next table or the actual page in it.
 * @param pageToLoad number of page we try to load on the RAM
 * @param forbiddenFrame number of frame, which we got from the previous call for findNextEmptyFrame, so we cannot pick it.
 * @return number of an empty and valid frame
 */
word_t findNextEmptyFrame(const uint64_t &pageToLoad, const uint64_t& forbiddenFrame) {
    uint64_t maxCyclicDist = 0, maxFrameAllocated = 0, pageToEvict = 0, frameToEvict = 0, parentOfEvicted = 0;

    // the next frame search:
    uint64_t nextFrame = dfs(pageToLoad, forbiddenFrame, maxFrameAllocated,
                             maxCyclicDist, pageToEvict, frameToEvict, parentOfEvicted,
                             0, 0, 0, 0);

    // case1 - dfs found an empty frame for the next table
    if (nextFrame != 0) return nextFrame;

    // case2 - there is e free space in RAM for a new frame
    if (maxFrameAllocated < NUM_FRAMES - 1) return maxFrameAllocated+1;

    // case 3 - eviction must take place
    PMwrite(parentOfEvicted, 0);
    PMevict(frameToEvict, pageToEvict);
    clearTable(frameToEvict);
    return frameToEvict;
}

/**
 * Convert virtual address to physical one
 * @param virtualAddress an address (word sized VIRTUAL_ADDRESS_WIDTH bits) of the virtual memory
 * @return a physical address (word sized PHYSICAL_ADDRESS_WIDTH bits) of the RAM
 */
uint64_t virtual2PhysicalAddressConversion(uint64_t virtualAddress) {
    uint64_t currPhysicalWord=0, forbiddenFrame=NUM_FRAMES, wordOffset = virtualAddress & PAGE_WIDTH_MASK;

    for (int level = 0; level < TABLES_DEPTH; level++) {
        // (addr1 * PAGE_SIZE) + pageIdx
        currPhysicalWord = (currPhysicalWord << OFFSET_WIDTH) + Ith_TABLE_VAL(level, virtualAddress);

        word_t nextTable = 0;
        PMread(currPhysicalWord, &nextTable);

        // the next table not in RAM, so we search new frame to contain it.
        if(nextTable == 0) {
            nextTable = findNextEmptyFrame(virtualAddress >> OFFSET_WIDTH, forbiddenFrame);
            forbiddenFrame = nextTable;
            PMwrite(currPhysicalWord, nextTable);

            // after finding a frame for the page itself, the page should be restored to it
            if (level==TABLES_DEPTH-1)
              PMrestore(nextTable, virtualAddress >> OFFSET_WIDTH);
        }
        currPhysicalWord = (uint64_t) nextTable;
    }

    // return the &page[word] address on the physical memory
    return TABLE_AT_I(currPhysicalWord, wordOffset);
}

/* =================================================== Header-Imp =================================================== */
void VMinitialize() {
    for (uint64_t i = 0; i < PAGE_SIZE; ++i) {
        PMwrite(i, 0);
    }
}


int VMread(uint64_t virtualAddress, word_t* value) {
    if (virtualAddress >= VIRTUAL_MEMORY_SIZE) {
        return 0;
    }
    uint64_t physicalAddress = virtual2PhysicalAddressConversion(virtualAddress);
    PMread(physicalAddress, value);
    return 1;
}


int VMwrite(uint64_t virtualAddress, word_t value) {
    if (virtualAddress >= VIRTUAL_MEMORY_SIZE) {
        return 0;
    }
    uint64_t physicalAddress = virtual2PhysicalAddressConversion(virtualAddress);
    PMwrite(physicalAddress, value);
    return 1;
}


