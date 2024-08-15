#pragma once

#ifndef LPQENGINE_H
#define LPQENGINE_H

#include "LPQTypes.h"

#ifdef LPQENGINE_EXPORT_CONSTANT
	#define LPQ_API __declspec(dllexport)
#else
	#define LPQ_API __declspec(dllimport)
#endif

#ifdef _DEBUG
	#define ALLOCATOR __declspec(allocator)
#else
	#define ALLOCATOR
#endif

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct _lpqFileInfo {
		uint8* pFileName;
		uint64 pFileSize;
		uint32 hashSum;
	} LPQFileInfo;

	typedef struct _lpqBuilder {
		LPQFileInfo* pLpqInfo;
		uint32 fileCount;
		uint32 modelCount;
	} LPQBuilder;

	typedef struct _lpqEngine {
		LPQBuilder* pLpqBuilder;
	} LPQEngine;

	LPQ_API ALLOCATOR LPQEngine* CreateEngine();
	LPQ_API void AnalizeTreeFiles(LPQBuilder** pBuilder, const char* path, const char** ignoreFiles, int size);
	LPQ_API void delete_engine(LPQEngine** ppEngine);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif