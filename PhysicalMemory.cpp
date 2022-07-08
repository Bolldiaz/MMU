#include "PhysicalMemory.h"


std::vector<page_t> RAM;
std::unordered_map<uint64_t, page_t> swapFile;

std::vector<page_t> getRAM() { return RAM; }
std::unordered_map<uint64_t, page_t> getSWAP(){ return swapFile; }

void printRAM() {
    std::cout << "##### RAM ####" << std::endl;
    int count=0;
    for (auto c: RAM) {
        std::cout << "Frame " << count++ << ":\t";
        for (auto r:c) {
            std::cout << r << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

void printSWAP() {
    std::cout << "##### SWAP ####" << std::endl;
    for (auto c: swapFile) {
        std::cout << "Page " << c.first << ":\t";
        for (auto r:c.second) {
            std::cout << r << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

void initialize() {
    RAM.resize(NUM_FRAMES, page_t(PAGE_SIZE));
}

void PMread(uint64_t physicalAddress, word_t* value) {
    if (RAM.empty()) {
        initialize();
    }

    assert(physicalAddress < RAM_SIZE);

    uint64_t frameIndex = physicalAddress / PAGE_SIZE;
    uint64_t frameOffset = physicalAddress % PAGE_SIZE;
    *value = RAM[frameIndex][frameOffset];
 }

void PMwrite(uint64_t physicalAddress, word_t value) {
    if (RAM.empty()) {
        initialize();
    }

    assert(physicalAddress < RAM_SIZE);

    uint64_t frameIndex = physicalAddress / PAGE_SIZE;
    uint64_t frameOffset = physicalAddress % PAGE_SIZE;
    RAM[frameIndex][frameOffset] = value;
}

void PMevict(uint64_t frameIndex, uint64_t evictedPageIndex) {
    if (RAM.empty()) {
        initialize();
    }
//    std::cout << "Evicted page: " << evictedPageIndex << ", Evicted frame: " << frameIndex <<std::endl;

    assert(frameIndex < NUM_FRAMES);
    assert(evictedPageIndex < NUM_PAGES);
    assert(swapFile.find(evictedPageIndex) == swapFile.end());

    swapFile[evictedPageIndex] = RAM[frameIndex];
}

void PMrestore(uint64_t frameIndex, uint64_t restoredPageIndex) {
    if (RAM.empty()) {
        initialize();
    }

    assert(frameIndex < NUM_FRAMES);

    // page is not in swap file, so this is essentially
    // the first reference to this page. we can just return
    // as it doesn't matter if the page contains garbage
    if (swapFile.find(restoredPageIndex) == swapFile.end()) {
        return;
    }

    RAM[frameIndex] = std::move(swapFile[restoredPageIndex]);
    swapFile.erase(restoredPageIndex);
}
