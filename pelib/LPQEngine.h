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

	enum file_type {
		FT_MESH		= 1 << 0,
		FT_TEXTURE	= 1 << 1,
		FT_MODEL	= 1 << 2,
		FT_SHADER	= 1 << 3,
		FT_LUA		= 1 << 4,
		FT_SCENE	= 1 << 5
	};

	struct LPQFileInfo;

	typedef struct _lpqFileInfo {
		uint8* pFileName;
		struct _lpqFileInfo* pFileAdditionaly;
		uint64 pFileSize;
		uint32 countAdditionalyFiles;
		enum file_type fileType;
	} LPQFileInfo;

	typedef struct _lpqBuilder {
		LPQFileInfo* pLpqInfo;
		uint32 fileCount;
	} LPQBuilder;

	typedef struct _lpqEngine {
		LPQBuilder* pLpqBuilder;
	} LPQEngine;

	LPQ_API ALLOCATOR LPQEngine* CreateEngine();
	LPQ_API void AnalizeTreeFiles(LPQBuilder** pBuilder, const char* path);
	LPQ_API void delete_engine(LPQEngine** ppEngine);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif