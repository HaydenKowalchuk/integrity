#pragma once

#include <stdbool.h>
#include <stdio.h>

#define FS_MAX_PATHS 6
#define FS_MAX_PATH 128

/**
 * Adds a directory to the search chain.
 * Paths are checked in the order they are added.
 */
void FS_AddSearchPath(const char* path);

/**
 * Searches the registered paths for 'filename'.
 * If found, 'out_path' is filled with the full resolved path.
 * Returns true if the file was found, false otherwise.
 */
bool FS_ResolvePath(const char* filename, char* out_path);

/**
 * Returns the size of an open file in bytes.
 */
int FS_FileLength(FILE* f);

/**
 * Clears all registered search paths.
 */
void FS_ClearSearchPaths(void);

/**
 * Returns true if a resolved path leads to a file
 */
bool FS_FileExists(const char* path);

const char* FS_ResolvePathTemp(const char* filename);

bool FS_DirExists(const char* path);
