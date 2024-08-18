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
} Vertex;

typedef struct {
    int v[3];
} Face;

typedef struct _model {
    Vertex* vertices;
    Face* faces;
    int vertex_count;
    int face_count;
    int textureCount;
    uint8** textures;
    uint8* fileName;
    uint8* root;
} Model;

ALLOCATOR Model* load_obj(const char* filename);
int save_model_binary(const char* filename, Model* model);
void load_model(Model** models, LPQFileInfo* pLpqFileInfo, int countFiles);
int compress_file(const char* input_file, const char* output_bhl, const char* output_lpq, const char* file_name);
void save_model(const Model* model);
void debug_model(const Model* model);
void delete_model(Model* model);

#endif