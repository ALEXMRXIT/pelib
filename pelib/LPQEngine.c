#include <stdio.h>
#include <stdlib.h>

#define USE_MEMORY_LEAK_DETECTED
#include "Memory.h"

#include "LPQEngine.h"
#include "Stack.h"

#define NENGINE_INSTANCE	1

LPQEngine* CreateEngine() {
    LPQFileInfo* pLpqInfo = (LPQFileInfo*)calloc(NENGINE_INSTANCE, sizeof(LPQFileInfo));
    if (!pLpqInfo) {
        fprintf(stderr, "Memory allocation failed for pLpqInfo.\n");
        return NULL;
    }
    LPQBuilder* pLpqBuilder = (LPQBuilder*)calloc(NENGINE_INSTANCE, sizeof(LPQBuilder));
    if (!pLpqBuilder) {
        fprintf(stderr, "Memory allocation failed for pLpqBuilder.\n");
        free(pLpqInfo);
        return NULL;
    }
    LPQEngine* pLpqEngine = (LPQEngine*)calloc(NENGINE_INSTANCE, sizeof(LPQEngine));
    if (!pLpqEngine) {
        fprintf(stderr, "Memory allocation failed for pLpqEngine.\n");
        free(pLpqBuilder);
        free(pLpqInfo);
        return NULL;
    }

    pLpqInfo->hashSum = 0;
    pLpqInfo->pFileName = NULL;
    pLpqInfo->pFileSize = 0;

    pLpqBuilder->pLpqInfo = pLpqInfo;
    pLpqBuilder->modelCount = 0;

    pLpqEngine->pLpqBuilder = pLpqBuilder;

    return pLpqEngine;
}

void addFileInfo(LPQBuilder* pBuilder, const char* fileName, uint64 fileSize, uint32 hashSum) {
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

    size_t length = strlen(fileName);
    if (strcmp((char*)fileName + length - 4, ".obj") == 0)
        pBuilder->modelCount++;

    pBuilder->pLpqInfo[index].pFileSize = fileSize;
    pBuilder->pLpqInfo[index].hashSum = hashSum;
}

void AnalizeTreeFiles(LPQBuilder** pBuilder, const char* path, const char** ignoreFiles, int size) {
    Stack stack = { .top = NULL };
    stackPush(&stack, path);

    (*pBuilder)->pLpqInfo = NULL;
    (*pBuilder)->fileCount = 0;

    while (stack.top != NULL) {
        char* currentPath = stackPop(&stack);
        if (currentPath == NULL)
            continue;

        WIN32_FIND_DATAA findFileData;
        HANDLE hFind;
        char fullPath[MAX_PATH_LENGTH];

        snprintf(fullPath, MAX_PATH_LENGTH, "%s\\*", currentPath);

        hFind = FindFirstFileA(fullPath, &findFileData);
        if (hFind == INVALID_HANDLE_VALUE) {
            printf("FindFirstFile failed (%d)\n", GetLastError());
            free(currentPath);
            continue;
        }

        do {
            if (strcmp(findFileData.cFileName, ".") == 0 || strcmp(findFileData.cFileName, "..") == 0)
                continue;

            if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                char breakCode = 0;
                for (int iterator = 0; iterator < size; ++iterator) {
                    char* point = strrchr(findFileData.cFileName, '.');
                    size_t size_p = strlen(point);
                    if (strncmp(point, ignoreFiles[iterator], size_p) == 0) {
                        breakCode = 1;
                        break;
                    }
                }
                if (breakCode) continue;
            }

            snprintf(fullPath, MAX_PATH_LENGTH, "%s\\%s", currentPath, findFileData.cFileName);

            if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                stackPush(&stack, fullPath);
            else {
                uint64 fileSize = findFileData.nFileSizeLow | ((uint64)findFileData.nFileSizeHigh << 32);
                uint32 hashSum = 0;
                addFileInfo(*pBuilder, fullPath, fileSize, hashSum);
            }
        } while (FindNextFileA(hFind, &findFileData) != 0);

        FindClose(hFind);
        free(currentPath);
    }

    stackClear(&stack);
}

void delete_engine(LPQEngine** ppEngine) {
    if (ppEngine == NULL || *ppEngine == NULL)
        return;

    LPQEngine* pEngine = *ppEngine;
    LPQBuilder* pBuilder = pEngine->pLpqBuilder;
    if (pBuilder != NULL) {
        printf("Deleting engine with %u files\n", pBuilder->fileCount);
        for (uint32 i = 0; i < pBuilder->fileCount; ++i) {
            LPQFileInfo* pInfo = &pBuilder->pLpqInfo[i];
            if (pInfo->pFileName != NULL) {
                printf("Freeing file name: %s\n", pInfo->pFileName);
                free(pInfo->pFileName);
            }
        }
        free(pBuilder->pLpqInfo);
        free(pBuilder);
    }
    free(pEngine);
    *ppEngine = NULL;
}