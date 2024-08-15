#include "Memory.h"

void add_memory_block(void* ptr, size_t size, const char* file, int line, const char* function) {
    MemoryBlock* block = (MemoryBlock*)malloc(sizeof(MemoryBlock));
    if (block == NULL) {
        MessageBox(NULL, "Out of memory", "Error", MB_ICONERROR);
        exit(1);
    }
    block->ptr = ptr;
    block->size = size;
    block->file = file;
    block->line = line;
    block->function = function;
    block->next = memory_list;
    memory_list = block;
}

void remove_memory_block(void* ptr) {
    MemoryBlock* current = memory_list;
    MemoryBlock* prev = NULL;

    while (current != NULL) {
        if (current->ptr == ptr) {
            if (prev == NULL)
                memory_list = current->next;
            else
                prev->next = current->next;
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}

void check_memory_leaks() {
    if (memory_list != NULL) {
        char message[1024] = "Memory leaks detected:\n";
        size_t msg_len = strlen(message);
        MemoryBlock* current = memory_list;
        while (current != NULL) {
            char block_info[256];
            int len = snprintf(block_info, sizeof(block_info), "File: %s, Line: %d, Function: %s, Size: %zu\n",
                current->file, current->line, current->function, current->size);
            if (len > 0 && msg_len + len < sizeof(message)) {
                strncat(message + msg_len, block_info, sizeof(message) - msg_len - 1);
                msg_len += len;
            }
            current = current->next;
        }
        MessageBoxA(NULL, message, "Memory Leak", MB_ICONERROR);
    }
}

void* my_malloc(size_t size, const char* file, int line, const char* function) {
    void* ptr = malloc(size);
    if (ptr == NULL) {
        MessageBox(NULL, "Out of memory", "Error", MB_ICONERROR);
        exit(1);
    }
    add_memory_block(ptr, size, file, line, function);
    return ptr;
}

void* my_calloc(size_t num, size_t size, const char* file, int line, const char* function) {
    void* ptr = calloc(num, size);
    if (ptr == NULL) {
        MessageBox(NULL, "Out of memory", "Error", MB_ICONERROR);
        exit(1);
    }
    add_memory_block(ptr, num * size, file, line, function);
    return ptr;
}

void* my_realloc(void* ptr, size_t size, const char* file, int line, const char* function) {
    void* new_ptr = realloc(ptr, size);
    if (new_ptr == NULL) {
        MessageBox(NULL, "Out of memory", "Error", MB_ICONERROR);
        exit(1);
    }
    remove_memory_block(ptr);
    add_memory_block(new_ptr, size, file, line, function);
    return new_ptr;
}

void my_free(void* ptr) {
    free(ptr);
    remove_memory_block(ptr);
}