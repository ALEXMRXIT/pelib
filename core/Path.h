#pragma once

#ifndef PATH_HEADER_H
#define PATH_HEADER_H

#ifdef CORE_CONSTANTS
#define CORE_API __declspec(dllexport)
#else
#define CORE_API __declspec(dllimport)
#endif

#include "LPQTypes.h"

#include <stdlib.h>
#include <stdio.h>
#include <io.h>

CORE_API char* getRootPath(const char* executablePath);
CORE_API char* addFolderToPath(const char* rootPath, const char* folder);
CORE_API void strip_path_and_extension(const char* filename, char* result, size_t result_size);
CORE_API void extract_path(const char* full_path, char* result, size_t result_size);
CORE_API uint8 file_remove(const char* path);

#endif