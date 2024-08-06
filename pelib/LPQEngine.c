#include <stdio.h>
#include <stdlib.h>

#include "LPQEngine.h"
#include "Stack.h"

#define NENGINE_INSTANCE	1

LPQEngine* CreateEngine() {
	LPQFileInfo* pLpqInfo = (LPQFileInfo*)calloc(NENGINE_INSTANCE, sizeof(LPQFileInfo));
	if (!pLpqInfo)
		return NULL;
	LPQBuilder* pLpqBuilder = (LPQBuilder*)calloc(NENGINE_INSTANCE, sizeof(LPQBuilder));
	if (!pLpqBuilder) {
		free(pLpqInfo);
		return NULL;
	}
	LPQEngine* pLpqEngine = (LPQEngine*)calloc(NENGINE_INSTANCE, sizeof(LPQEngine));
	if (!pLpqEngine) {
		free(pLpqBuilder);
		free(pLpqInfo);
		return NULL;
	}

	pLpqInfo->hashSum = 0;
	pLpqInfo->pFileName = NULL;
	pLpqInfo->pFileSize = 0;

	pLpqBuilder->pLpqInfo = pLpqInfo;
	pLpqEngine->pLpqBuilder = pLpqBuilder;

	return pLpqBuilder;
}

void addFileInfo(LPQBuilder* pBuilder, const char* fileName, uint64 fileSize, uint32 hashSum) {
	pBuilder->fileCount++;
	pBuilder->pLpqInfo = (LPQFileInfo*)realloc(pBuilder->pLpqInfo, pBuilder->fileCount * sizeof(LPQFileInfo));
	if (pBuilder->pLpqInfo == NULL) {
		fprintf(stderr, "Memory allocation failed.\n");
		return;
	}

	int index = pBuilder->fileCount - 1;
	pBuilder->pLpqInfo[index].pFileName = (uint8*)malloc(strlen(fileName) + 1);
	if (pBuilder->pLpqInfo[index].pFileName == NULL) {
		fprintf(stderr, "Memory allocation failed.\n");
		return;
	}
	strcpy((char*)pBuilder->pLpqInfo[index].pFileName, fileName);

	pBuilder->pLpqInfo[index].pFileSize = fileSize;
	pBuilder->pLpqInfo[index].hashSum = hashSum;
}

void AnalizeTreeFiles(LPQBuilder** pBuilder, const char* path) {
	Stack stack = { .top = NULL };
	stackPush(&stack, path);

	LPQBuilder* builder = (LPQBuilder*)malloc(sizeof(LPQBuilder));
	if (builder == NULL) {
		fprintf(stderr, "Memory allocation failed.\n");
		return;
	}
	builder->pLpqInfo = NULL;
	builder->fileCount = 0;

	while (stack.top != NULL) {
		char* path = stackPop(&stack);
		WIN32_FIND_DATAA findFileData;
		HANDLE hFind;
		char fullPath[MAX_PATH_LENGTH];

		snprintf(fullPath, sizeof(fullPath), "%s\\*", path);

		hFind = FindFirstFileA(fullPath, &findFileData);
		if (hFind == INVALID_HANDLE_VALUE) {
			printf("FindFirstFile failed (%d)\n", GetLastError());
			continue;
		}

		do {
			if (strcmp(findFileData.cFileName, ".") == 0 || strcmp(findFileData.cFileName, "..") == 0)
				continue;

			snprintf(fullPath, sizeof(fullPath), "%s\\%s", path, findFileData.cFileName);

			if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				stackPush(&stack, fullPath);
			}
			else {
				uint64 fileSize = findFileData.nFileSizeLow | ((uint64)findFileData.nFileSizeHigh << 32);
				uint32 hashSum = 0;
				addFileInfo(builder, fullPath, fileSize, hashSum);
			}
		} while (FindNextFileA(hFind, &findFileData) != 0);

		FindClose(hFind);
	}

	*pBuilder = builder;
}
