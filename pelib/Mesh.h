#pragma once

#ifndef MESH_HEADER_H
#define MESH_HEADER_H

#ifdef _DEBUG
#define ALLOCATOR __declspec(allocator)
#else
#define ALLOCATOR
#endif

#include "LPQEngine.h"
#include "LPQTypes.h"

typedef struct {
    float x, y, z;
    float u, v;
    float nx, ny, nz;
} Vertex;

typedef struct {
    int v[3];
    int t[3];
    int n[3];
} Face;

typedef struct _model {
    Vertex* vertices;
    Face* faces;
    int vertex_count;
    int face_count;
    int texture_count;
    uint8** textures;
    uint8* fileName;
    uint8* root;
    float* texcoords;
    float* normals;
} Model;

LPQ_API ALLOCATOR Model* load_obj(const char* filename);
LPQ_API int save_model_binary(const char* filename, Model* model);
LPQ_API void load_model(Model** models, LPQFileInfo* pLpqFileInfo, int countFiles);
LPQ_API int compress_file(const char* input_file, const char* output_bhl, const char* output_lpq, const char* file_name);
LPQ_API void save_model(const Model* model);
LPQ_API void debug_model(const Model* model);
LPQ_API void delete_model(Model* model);
LPQ_API void lpq_load_header(BHLFileEnty** pBhlFileEntry);
LPQ_API Model* lpq_extract_file_model(const char* filename);

#endif