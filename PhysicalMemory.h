#pragma once

#include "MemoryConstants.h"
#include <vector>
#include <unordered_map>
#include <cassert>
#include <cstdio>
#include <iostream>

typedef std::vector<word_t> page_t;
/*
 * Reads an integer from the given physical address and puts it in 'value'.
 */
void PMread(uint64_t physicalAddress, word_t* value);

/*
 * Writes 'value' to the given physical address.
 */
void PMwrite(uint64_t physicalAddress, word_t value);


/*
 * Evicts a page from the RAM to the hard drive.
 */
void PMevict(uint64_t frameIndex, uint64_t evictedPageIndex);


/*
 * Restores a page from the hard drive to the RAM.
 */
void PMrestore(uint64_t frameIndex, uint64_t restoredPageIndex);

void printSWAP();

void printRAM();

std::vector<page_t> getRAM();
std::unordered_map<uint64_t, page_t> getSWAP();
