#include "Mesh.h"
#include "Path.h"

#define USE_MEMORY_LEAK_DETECTED
#include "Memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>

#define CHUNK 16384

Model* load_obj(const char* filename) {
    Model* model = (Model*)malloc(sizeof(Model));
    if (!model) return NULL;

    model->vertex_count = 0;
    model->face_count = 0;
    model->texture_count = 0;
    model->textures = NULL;
    model->fileName = NULL;
    model->root = NULL;
    model->texcoords = NULL;
    model->normals = NULL;

    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        free(model);
        return NULL;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == 'v' && line[1] == ' ')
            model->vertex_count++;
        else if (line[0] == 'f' && line[1] == ' ')
            model->face_count++;
    }

    model->vertices = (Vertex*)malloc(model->vertex_count * sizeof(Vertex));
    model->faces = (Face*)malloc(model->face_count * sizeof(Face));
    model->texcoords = (float*)malloc(model->vertex_count * 2 * sizeof(float));
    model->normals = (float*)malloc(model->vertex_count * 3 * sizeof(float));

    if (!model->vertices || !model->faces || !model->texcoords || !model->normals) {
        free(model->vertices);
        free(model->faces);
        free(model->texcoords);
        free(model->normals);
        free(model);
        fclose(file);
        return NULL;
    }

    rewind(file);

    int vertex_index = 0;
    int face_index = 0;
    int texcoord_index = 0;
    int normal_index = 0;

    while (fgets(line, sizeof(line), file)) {
        if (line[0] == 'v' && line[1] == ' ') {
            sscanf(line, "v %f %f %f", &model->vertices[vertex_index].x, &model->vertices[vertex_index].y, &model->vertices[vertex_index].z);
            vertex_index++;
        }
        else if (line[0] == 'v' && line[1] == 't') {
            sscanf(line, "vt %f %f", &model->texcoords[texcoord_index], &model->texcoords[texcoord_index + 1]);
            texcoord_index += 2;
        }
        else if (line[0] == 'v' && line[1] == 'n') {
            sscanf(line, "vn %f %f %f", &model->normals[normal_index], &model->normals[normal_index + 1], &model->normals[normal_index + 2]);
            normal_index += 3;
        }
        else if (line[0] == 'f' && line[1] == ' ') {
            sscanf(line, "f %d/%d/%d %d/%d/%d %d/%d/%d",
                &model->faces[face_index].v[0], &model->faces[face_index].t[0], &model->faces[face_index].n[0],
                &model->faces[face_index].v[1], &model->faces[face_index].t[1], &model->faces[face_index].n[1],
                &model->faces[face_index].v[2], &model->faces[face_index].t[2], &model->faces[face_index].n[2]);
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
    fwrite(&model->texture_count, sizeof(int), 1, file);

    fwrite(model->texcoords, sizeof(float), model->vertex_count * 2, file);

    fwrite(model->normals, sizeof(float), model->vertex_count * 3, file);

    for (int i = 0; i < model->texture_count; i++) {
        int texture_length = strlen((char*)model->textures[i]) + 1;
        fwrite(&texture_length, sizeof(int), 1, file);
        fwrite(model->textures[i], sizeof(uint8), texture_length, file);
    }

    fclose(file);
    return 1;
}

void load_model(Model** models, const LPQFileInfo* pLpqFileInfo, int countFiles) {
    uint8* file = pLpqFileInfo->pFileName;
    size_t length = strlen((char*)file);
    if (strcmp((char*)file + length - 4, ".obj") == 0) {
        Model* model = load_obj((char*)file);
        if (model) {
            const int pathSize = 128;
            model->fileName = (uint8*)malloc(pathSize * sizeof(uint8));
            model->root = (uint8*)malloc(pathSize * sizeof(uint8));
            strip_path_and_extension((char*)file, (char*)model->fileName, pathSize);
            extract_path((char*)file, (char*)model->root, pathSize);

            char modelName[64];
            strip_path_and_extension((char*)file, modelName, sizeof(modelName));
            model->textureCount = 0;
            for (int i = 0; i < countFiles; ++i) {
                file = pLpqFileInfo->pFileAdditionaly[i].pFileName;
                length = strlen((char*)file);
                if (strcmp((char*)file + length - 4, ".png") == 0) {
                    char textureName[64];
                    strip_path_and_extension((char*)file, textureName, sizeof(textureName));
                    size_t len = strlen(modelName);
                    if (strncmp(modelName, textureName, len) == 0) {
                        model->textures = (uint8**)realloc(model->textures, (model->textureCount + 1) * sizeof(uint8*));
                        if (!model->textures) {
                            perror("Failed to reallocate memory for textures");
                            return;
                        }

                        model->textures[model->textureCount] = (uint8*)malloc((strlen(textureName) + 1) * sizeof(uint8));
                        if (!model->textures[model->textureCount]) {
                            perror("Failed to allocate memory for texture name");
                            return;
                        }

                        strcpy((char*)model->textures[model->textureCount], textureName);
                        model->textureCount++;
                    }
                }
            }
            *models = model;
        }
    }
}

unsigned int simple_hash(const char* str) {
    unsigned int hash = 0;
    while (*str)
        hash = (hash << 5) ^ (hash >> 27) ^ (*str++);
    return hash;
}

int compress_file(const char* input_file, const char* output_bhl, const char* output_lpq, const char* file_name) {
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

    unsigned int hash = simple_hash(file_name);
    fwrite(&hash, sizeof(unsigned int), 1, bhl_file);

    fwrite(&original_size, sizeof(long), 1, bhl_file);
    fwrite(&compressed_size, sizeof(unsigned long), 1, bhl_file);

    FILE* lpq_file = fopen(output_lpq, "ab");
    if (!lpq_file) {
        perror("Failed to open output lpq file");
        free(in_buffer);
        free(out_buffer);
        fclose(bhl_file);
        return -1;
    }

    fseek(lpq_file, 0, SEEK_END);
    long start_position = ftell(lpq_file);
    fwrite(out_buffer, 1, compressed_size, lpq_file);
    fclose(lpq_file);

    fwrite(&start_position, sizeof(long), 1, bhl_file);

    fclose(bhl_file);

    free(in_buffer);
    free(out_buffer);

    return 0;
}

void save_texture(const char* texture_file, const char* output_bhl, const char* output_lpq, int file_count) {
    FILE* texture_infile = fopen(texture_file, "rb");
    if (!texture_infile) {
        perror("Failed to open texture file");
        return;
    }

    fseek(texture_infile, 0, SEEK_END);
    long texture_original_size = ftell(texture_infile);
    fseek(texture_infile, 0, SEEK_SET);

    unsigned char* texture_buffer = (unsigned char*)malloc(texture_original_size);
    if (!texture_buffer) {
        perror("Failed to allocate memory for texture buffer");
        fclose(texture_infile);
        return;
    }

    fread(texture_buffer, 1, texture_original_size, texture_infile);
    fclose(texture_infile);

    compress_file(texture_file, output_bhl, output_lpq, file_count);

    free(texture_buffer);
}

void save_model(const Model* model) {
    char filename[256];
    snprintf(filename, sizeof(filename), "%s\\%s.model", model->root, model->fileName);
    if (save_model_binary(filename, model)) {
        printf("Model saved to %s\n", filename);

        char output_bhl[128];
        char output_lpq[128];
        char* root = getRootPath(model->root);
        snprintf(output_bhl, sizeof(output_bhl), "%s\\mesh.bhl", root);
        snprintf(output_lpq, sizeof(output_lpq), "%s\\mesh.lpq", root);
        int total_files = model->textureCount + 1;

        FILE* bhl_file = fopen(output_bhl, "rb+");
        if (!bhl_file) {
            bhl_file = fopen(output_bhl, "wb");
            if (!bhl_file) {
                perror("Failed to open output bhl file for writing");
                return;
            }
            fwrite(&total_files, sizeof(int), 1, bhl_file);
        }
        else {
            int current_total_files;
            fread(&current_total_files, sizeof(int), 1, bhl_file);
            total_files += current_total_files;
            fseek(bhl_file, 0, SEEK_SET);
            fwrite(&total_files, sizeof(int), 1, bhl_file);
        }

        fclose(bhl_file);

        if (compress_file(filename, output_bhl, output_lpq, model->fileName) != 0)
            printf("Failed to compress model %s\n", model->fileName);
        else
            printf("Model compressed and saved to %s and %s\n", output_bhl, output_lpq);

        for (int i = 0; i < model->textureCount; ++i) {
            char texture_filename[256];
            snprintf(texture_filename, sizeof(texture_filename), "%s\\%s.png", model->root, model->textures[i]);
            save_texture(texture_filename, output_bhl, output_lpq, texture_filename);
        }
    }
    else
        printf("Failed to save model %s\n", filename);
    remove(filename);
}

void debug_model(const Model* model) {
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