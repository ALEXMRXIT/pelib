#include "Mesh.h"
#include "Path.h"

#define USE_MEMORY_LEAK_DETECTED
#include "Memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>

#define CHUNK 16384

ALLOCATOR Model* load_obj(const char* filename) {
    Model* model = (Model*)malloc(sizeof(Model));
    if (!model) return NULL;
    model->vertex_count = 0;
    model->face_count = 0;
    model->textures = NULL;
    model->root = NULL;
    model->fileName = NULL;

    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        return NULL;
    }

    char line[128];
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == 'v' && line[1] == ' ')
            model->vertex_count++;
        else if (line[0] == 'f' && line[1] == ' ')
            model->face_count++;
    }

    model->vertices = (Vertex*)malloc(model->vertex_count * sizeof(Vertex));
    model->faces = (Face*)malloc(model->face_count * sizeof(Face));

    if (!model->vertices) {
        free(model);
        return NULL;
    }
    if (!model->faces) {
        free(model);
        return NULL;
    }

    rewind(file);

    int vertex_index = 0;
    int face_index = 0;

    while (fgets(line, sizeof(line), file)) {
        if (line[0] == 'v' && line[1] == ' ') {
            sscanf(line, "v %f %f %f", &model->vertices[vertex_index].x, &model->vertices[vertex_index].y, &model->vertices[vertex_index].z);
            vertex_index++;
        }
        else if (line[0] == 'f' && line[1] == ' ') {
            sscanf(line, "f %d %d %d", &model->faces[face_index].v[0], &model->faces[face_index].v[1], &model->faces[face_index].v[2]);
            face_index++;
        }
    }

    fclose(file);
    return model;
}

int save_model_binary(const char* filename, Model* model) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        perror("Failed to open file for writing");
        return 0;
    }

    fwrite(&model->vertex_count, sizeof(int), 1, file);
    fwrite(model->vertices, sizeof(Vertex), model->vertex_count, file);
    fwrite(&model->face_count, sizeof(int), 1, file);
    fwrite(model->faces, sizeof(Face), model->face_count, file);
    fwrite(&model->textureCount, sizeof(int), 1, file);

    for (int i = 0; i < model->textureCount; i++) {
        int texture_length = strlen(model->textures[i]) + 1;
        fwrite(&texture_length, sizeof(int), 1, file);
        fwrite(model->textures[i], sizeof(uint8), texture_length, file);
    }

    fclose(file);
    return 1;
}

void load_model(Model** models, const LPQFileInfo* pLpqFileInfo, int countFiles) {
    uint8* file = pLpqFileInfo->pFileName;
    size_t length = strlen(file);
    if (strcmp((char*)file + length - 4, ".obj") == 0) {
        Model* model = load_obj(file);
        if (model) {
            const int pathSize = 128;
            model->fileName = (uint8*)malloc(pathSize * sizeof(uint8));
            model->root = (uint8*)malloc(pathSize * sizeof(uint8));
            strip_path_and_extension(file, model->fileName, pathSize);
            extract_path(file, model->root, pathSize);

            char modelName[64];
            strip_path_and_extension(file, modelName, sizeof(modelName));
            model->textureCount = 0;
            for (int i = 0; i < countFiles; ++i) {
                file = pLpqFileInfo[i].pFileName;
                length = strlen(file);
                if (strcmp(file + length - 4, ".png") == 0) {
                    char textureName[64];
                    strip_path_and_extension(file, textureName, sizeof(textureName));
                    size_t len = strlen(modelName);
                    if (strncmp(modelName, textureName, len) == 0) {
                        model->textures = (uint8**)realloc(model->textures, (model->textureCount + 1) * sizeof(uint8*));
                        if (!model->textures) {
                            perror("Failed to reallocate memory for textures");
                            return 0;
                        }

                        model->textures[model->textureCount] = (uint8*)malloc((strlen(textureName) + 1) * sizeof(uint8));
                        if (!model->textures[model->textureCount]) {
                            perror("Failed to allocate memory for texture name");
                            return 0;
                        }

                        strcpy(model->textures[model->textureCount], textureName);
                        model->textureCount++;
                    }
                }
            }
            *models = model;
        }
    }
}

int compress_file(const char* input_file, const char* output_bhl, const char* output_lpq, int file_count) {
    FILE* infile = fopen(input_file, "rb");
    if (!infile) {
        perror("Failed to open input file");
        return -1;
    }

    fseek(infile, 0, SEEK_END);
    long original_size = ftell(infile);
    fseek(infile, 0, SEEK_SET);

    unsigned char* in_buffer = (unsigned char*)malloc(original_size);
    if (!in_buffer) {
        perror("Failed to allocate memory for input buffer");
        fclose(infile);
        return -1;
    }

    fread(in_buffer, 1, original_size, infile);
    fclose(infile);

    unsigned long compressed_size = compressBound(original_size);
    unsigned char* out_buffer = (unsigned char*)malloc(compressed_size);
    if (!out_buffer) {
        perror("Failed to allocate memory for output buffer");
        free(in_buffer);
        return -1;
    }

    if (compress(out_buffer, &compressed_size, in_buffer, original_size) != Z_OK) {
        perror("Compression failed");
        free(in_buffer);
        free(out_buffer);
        return -1;
    }

    FILE* bhl_file = fopen(output_bhl, "ab");
    if (!bhl_file) {
        perror("Failed to open output bhl file");
        free(in_buffer);
        free(out_buffer);
        return -1;
    }

    if (file_count == 1)
        fwrite(&file_count, sizeof(int), 1, bhl_file);

    fwrite(&original_size, sizeof(long), 1, bhl_file);
    fwrite(&compressed_size, sizeof(unsigned long), 1, bhl_file);

    fclose(bhl_file);

    FILE* lpq_file = fopen(output_lpq, "ab");
    if (!lpq_file) {
        perror("Failed to open output lpq file");
        free(in_buffer);
        free(out_buffer);
        return -1;
    }

    fwrite(out_buffer, 1, compressed_size, lpq_file);
    fclose(lpq_file);

    free(in_buffer);
    free(out_buffer);

    return 0;
}

void save_model(Model* model) {
    char filename[256];
    snprintf(filename, sizeof(filename), "%s\\%s.model", model->root, model->fileName);
    if (save_model_binary(filename, model)) {
        printf("Model saved to %s\n", filename);

        char output_bhl[128];
        char output_lpq[128];
        char* root = getRootPath(model->root);
        snprintf(output_bhl, sizeof(output_bhl), "%s\\mesh.bhl", root);
        snprintf(output_lpq, sizeof(output_lpq), "%s\\mesh.lpq", root);

        if (compress_file(filename, output_bhl, output_lpq, 1) != 0)
            printf("Failed to compress model %s\n", model->fileName);
        else
            printf("Model compressed and saved to %s and %s\n", output_bhl, output_lpq);
        free(root);
    }
    else
        printf("Failed to save model %s\n", filename);
}

void debug_model(Model* model) {
    printf("Model: %d vertices, %d faces, %d textures\n", model->vertex_count, model->face_count, model->textureCount);
    for (int j = 0; j < model->textureCount; ++j)
        printf("|--Texture %d: %s\n", j, model->textures[j]);
}

void delete_model(Model* model) {
    free(model->vertices);
    free(model->faces);
    for (int j = 0; j < model->textureCount; j++)
        free(model->textures[j]);
    free(model->textures);
    free(model->fileName);
    free(model->root);
    free(model);
}