#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define USE_MEMORY_LEAK_DETECTED
#include "Memory.h"

#include "LPQEngine.h"
#include "Stack.h"

#define NENGINE_INSTANCE	1

LPQEngine* CreateEngine() {
    LPQBuilder* pLpqBuilder = (LPQBuilder*)calloc(NENGINE_INSTANCE, sizeof(LPQBuilder));
    if (!pLpqBuilder) {
        fprintf(stderr, "Memory allocation failed for pLpqBuilder.\n");
        return NULL;
    }
    LPQEngine* pLpqEngine = (LPQEngine*)calloc(NENGINE_INSTANCE, sizeof(LPQEngine));
    if (!pLpqEngine) {
        fprintf(stderr, "Memory allocation failed for pLpqEngine.\n");
        free(pLpqBuilder);
        return NULL;
    }

    pLpqBuilder->pLpqInfo = NULL;

    pLpqEngine->pLpqBuilder = pLpqBuilder;

    return pLpqEngine;
}

enum file_type getFileType(const char* extension) {
    if (strcmp(extension, ".obj") == 0) return FT_MESH;
    if (strcmp(extension, ".model") == 0) return FT_MODEL;
    if (strcmp(extension, ".fx") == 0) return FT_SHADER;
    if (strcmp(extension, ".lua") == 0) return FT_LUA;
    if (strcmp(extension, ".sce") == 0) return FT_SCENE;
    return 0;
}

void addFileInfo(LPQBuilder* pBuilder, const char* fileName, uint64 fileSize, enum file_type type) {
    pBuilder->fileCount++;
    LPQFileInfo* newInfo = (LPQFileInfo*)realloc(pBuilder->pLpqInfo, pBuilder->fileCount * sizeof(LPQFileInfo));
    if (newInfo == NULL) {
        fprintf(stderr, "Memory allocation failed for pLpqInfo.\n");
        return;
    }
    pBuilder->pLpqInfo = newInfo;

    int index = pBuilder->fileCount - 1;
    pBuilder->pLpqInfo[index].pFileName = (uint8*)malloc(strlen(fileName) + 1);
    if (pBuilder->pLpqInfo[index].pFileName == NULL) {
        fprintf(stderr, "Memory allocation failed for pFileName.\n");
        return;
    }
    strcpy((char*)pBuilder->pLpqInfo[index].pFileName, fileName);

    pBuilder->pLpqInfo[index].pFileSize = fileSize;
    pBuilder->pLpqInfo[index].fileType = type;
    pBuilder->pLpqInfo[index].countAdditionalyFiles = 0;
    pBuilder->pLpqInfo[index].pFileAdditionaly = NULL;
}

void addRelatedFile(LPQFileInfo* pInfo, const char* fileName, uint64 fileSize) {
    pInfo->countAdditionalyFiles++;
    LPQFileInfo* newAdditionaly = (LPQFileInfo*)realloc(pInfo->pFileAdditionaly, pInfo->countAdditionalyFiles * sizeof(LPQFileInfo));
    if (newAdditionaly == NULL) {
        fprintf(stderr, "Memory allocation failed for pFileAdditionaly.\n");
        return;
    }
    pInfo->pFileAdditionaly = newAdditionaly;

    int index = pInfo->countAdditionalyFiles - 1;
    pInfo->pFileAdditionaly[index].pFileName = (uint8*)malloc(strlen(fileName) + 1);
    if (pInfo->pFileAdditionaly[index].pFileName == NULL) {
        fprintf(stderr, "Memory allocation failed for pFileAdditionaly entry.\n");
        return;
    }
    strcpy((char*)pInfo->pFileAdditionaly[index].pFileName, fileName);
    pInfo->pFileAdditionaly[index].pFileSize = fileSize;
    pInfo->pFileAdditionaly[index].fileType = FT_TEXTURE;
    pInfo->pFileAdditionaly[index].countAdditionalyFiles = 0;
    pInfo->pFileAdditionaly[index].pFileAdditionaly = NULL;
}

void findAndAddRelatedFiles(LPQFileInfo* pInfo, const char* directory, const char* prefix) {
    WIN32_FIND_DATAA findFileData;
    HANDLE hFind;
    char searchPath[MAX_PATH];

    snprintf(searchPath, MAX_PATH, "%s\\*", directory);

    hFind = FindFirstFileA(searchPath, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        printf("FindFirstFile failed (%d)\n", GetLastError());
        return;
    }

    do {
        if (strcmp(findFileData.cFileName, ".") == 0 || strcmp(findFileData.cFileName, "..") == 0)
            continue;

        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            const char* extension = strrchr(findFileData.cFileName, '.');
            if (extension == NULL || strcmp(extension, ".png") != 0)
                continue;

            if (strncmp(findFileData.cFileName, prefix, strlen(prefix)) == 0) {
                char fullPath[MAX_PATH];
                snprintf(fullPath, MAX_PATH, "%s\\%s", directory, findFileData.cFileName);
                uint64 fileSize = findFileData.nFileSizeLow | ((uint64)findFileData.nFileSizeHigh << 32);

                addRelatedFile(pInfo, fullPath, fileSize);
            }
        }
    } while (FindNextFileA(hFind, &findFileData) != 0);

    FindClose(hFind);
}

void AnalizeTreeFilesRecursive(LPQBuilder* pBuilder, const char* path) {
    WIN32_FIND_DATAA findFileData;
    HANDLE hFind;
    char fullPath[MAX_PATH_LENGTH];

    snprintf(fullPath, MAX_PATH_LENGTH, "%s\\*", path);

    hFind = FindFirstFileA(fullPath, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        printf("FindFirstFile failed (%d)\n", GetLastError());
        return;
    }

    do {
        if (strcmp(findFileData.cFileName, ".") == 0 || strcmp(findFileData.cFileName, "..") == 0)
            continue;

        snprintf(fullPath, MAX_PATH_LENGTH, "%s\\%s", path, findFileData.cFileName);

        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            AnalizeTreeFilesRecursive(pBuilder, fullPath);
        }
        else {
            const char* extension = strrchr(findFileData.cFileName, '.');
            if (extension == NULL)
                continue;

            enum file_type type = getFileType(extension);
            if (type == 0)
                continue;

            uint64 fileSize = findFileData.nFileSizeLow | ((uint64)findFileData.nFileSizeHigh << 32);
            addFileInfo(pBuilder, fullPath, fileSize, type);

            if (type == FT_MESH) {
                int index = pBuilder->fileCount - 1;
                char prefix[MAX_PATH];
                strncpy(prefix, findFileData.cFileName, extension - findFileData.cFileName);
                prefix[extension - findFileData.cFileName] = '\0';
                findAndAddRelatedFiles(&pBuilder->pLpqInfo[index], path, prefix);
            }
        }
    } while (FindNextFileA(hFind, &findFileData) != 0);

    FindClose(hFind);
}

void AnalizeTreeFiles(LPQBuilder** pBuilder, const char* path) {
    (*pBuilder)->pLpqInfo = NULL;
    (*pBuilder)->fileCount = 0;

    AnalizeTreeFilesRecursive(*pBuilder, path);
}

void delete_engine(LPQEngine** ppEngine) {
    if (ppEngine == NULL || *ppEngine == NULL)
        return;

    LPQEngine* pEngine = *ppEngine;
    LPQBuilder* pBuilder = pEngine->pLpqBuilder;
    if (pBuilder != NULL) {
        for (uint32 i = 0; i < pBuilder->fileCount; ++i) {
            LPQFileInfo* pInfo = &pBuilder->pLpqInfo[i];
            if (pInfo->pFileName != NULL)
                free(pInfo->pFileName);
            if (pInfo->pFileAdditionaly != NULL) {
                for (uint32 j = 0; j < pInfo->countAdditionalyFiles; ++j) {
                    if (pInfo->pFileAdditionaly[j].pFileName != NULL)
                        free(pInfo->pFileAdditionaly[j].pFileName);
                }
                free(pInfo->pFileAdditionaly);
            }
        }
        free(pBuilder->pLpqInfo);
        free(pBuilder);
    }
    free(pEngine);
    *ppEngine = NULL;
}