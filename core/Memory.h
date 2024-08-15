#pragma once

#ifndef MEMORY_HEADER_H
#define MEMORY_HEADER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#ifdef _WIN32
#ifdef MY_ALLOCATOR_EXPORTS
#define MY_ALLOCATOR_API __declspec(dllexport)
#else
#define MY_ALLOCATOR_API __declspec(dllimport)
#endif
#else
#define MY_ALLOCATOR_API
#endif

#ifdef _DEBUG
#define ALLOCATOR __declspec(allocator)
#else
#define ALLOCATOR
#endif

#ifdef USE_MEMORY_LEAK_DETECTED
#define malloc(size) my_malloc(size, __FILE__, __LINE__, __FUNCTION__)
#define calloc(num, size) my_calloc(num, size, __FILE__, __LINE__, __FUNCTION__)
#define realloc(ptr, size) my_realloc(ptr, size, __FILE__, __LINE__, __FUNCTION__)
#define free(ptr) my_free(ptr)
#endif

typedef struct MemoryBlock {
    void* ptr;
    size_t size;
    const char* file;
    int line;
    const char* function;
    struct MemoryBlock* next;
} MemoryBlock;

static MemoryBlock* memory_list = NULL;

ALLOCATOR void add_memory_block(void* ptr, size_t size, const char* file, int line, const char* function);
void remove_memory_block(void* ptr);
MY_ALLOCATOR_API void check_memory_leaks();
MY_ALLOCATOR_API ALLOCATOR void* my_malloc(size_t size, const char* file, int line, const char* function);
MY_ALLOCATOR_API ALLOCATOR void* my_calloc(size_t num, size_t size, const char* file, int line, const char* function);
MY_ALLOCATOR_API ALLOCATOR void* my_realloc(void* ptr, size_t size, const char* file, int line, const char* function);
MY_ALLOCATOR_API void my_free(void* ptr);

#endif