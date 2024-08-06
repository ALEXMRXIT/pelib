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
int process_files(Model** models, LPQBuilder* pLpqBuilder);

#endif