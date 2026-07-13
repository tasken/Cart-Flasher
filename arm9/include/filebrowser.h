#pragma once

#include <cstddef>

// Lets the user navigate the SD card starting at startPath and pick a file
// whose name ends in ext (case-insensitive). Returns true and fills outPath
// with the full selected path, or false if the user backed out at the root.
bool BrowseForFile(const char* startPath, const char* ext, char* outPath, size_t outPathSize);
