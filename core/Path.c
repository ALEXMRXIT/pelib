#include "Path.h"

#define USE_MEMORY_LEAK_DETECTED
#include "Memory.h"

char* getRootPath(const char* executablePath) {
    char* path = _strdup(executablePath);
    if (path == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        return NULL;
    }
    char* lastSlash = strrchr(path, '/');
    if (lastSlash == NULL)
        lastSlash = strrchr(path, '\\');
    if (lastSlash != NULL)
        *lastSlash = '\0';
    return path;
}

char* addFolderToPath(const char* rootPath, const char* folder) {
    int size = strlen(rootPath) + strlen(folder) + 2;
    char* newPath = (char*)malloc(size);
    if (newPath == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        return NULL;
    }

    strcpy(newPath, rootPath);
    strcat(newPath, "\\");
    strcat(newPath, folder);

    return newPath;
}

void strip_path_and_extension(const char* filename, char* result, size_t result_size) {
    size_t len = strlen(filename);

    const char* last_dot = strrchr(filename, '.');
    if (!last_dot) last_dot = filename + len;

    const char* last_slash = strrchr(filename, '/');
    const char* last_backslash = strrchr(filename, '\\');
    if (last_slash == NULL && last_backslash == NULL)
        last_slash = filename;
    else {
        last_slash = (last_slash > last_backslash) ? last_slash : last_backslash;
        last_slash++;
    }

    size_t result_len = last_dot - last_slash;
    if (result_len >= result_size)
        result_len = result_size - 1;
    strncpy(result, last_slash, result_len);
    result[result_len] = '\0';
}

void extract_path(const char* full_path, char* result, size_t result_size) {
    size_t len = strlen(full_path);

    const char* last_slash = strrchr(full_path, '/');
    const char* last_backslash = strrchr(full_path, '\\');
    if (last_slash == NULL && last_backslash == NULL)
        result[0] = '\0';
    else {
        last_slash = (last_slash > last_backslash) ? last_slash : last_backslash;
        size_t result_len = last_slash - full_path;
        if (result_len >= result_size)
            result_len = result_size - 1;
        strncpy(result, full_path, result_len);
        result[result_len] = '\0';
    }
}

uint8 file_remove(const char* path) {
    return (_access(path, 0) == 0 && remove(path));
}