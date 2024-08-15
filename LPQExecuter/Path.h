#pragma once

#ifndef PATH_HEADER_H
#define PATH_HEADER_H

#include "LPQTypes.h"

#include <stdlib.h>
#include <stdio.h>
#include <io.h>

char* getRootPath(const char* executablePath);
char* addFolderToPath(const char* rootPath, const char* folder);
void strip_path_and_extension(const char* filename, char* result, size_t result_size);
void extract_path(const char* full_path, char* result, size_t result_size);
uint8 file_remove(const char* path);

#endif