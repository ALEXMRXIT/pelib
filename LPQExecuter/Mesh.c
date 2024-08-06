#include "Mesh.h"

#include <stdio.h>
#include <stdlib.h>

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

void strip_path_and_extension(const char* filename, char* result, size_t result_size) {
    size_t len = strlen(filename);

    const char* last_dot = strrchr(filename, '.');
    if (!last_dot) last_dot = filename + len;

    const char* last_slash = strrchr(filename, '/');
    const char* last_backslash = strrchr(filename, '\\');
    if (last_slash == NULL && last_backslash == NULL)
        last_slash = filename;
    else {
        last_slash = (last_slash > last_backslash) ? last_slash : last_backslash;
        last_slash++;
    }

    size_t result_len = last_dot - last_slash;
    if (result_len >= result_size)
        result_len = result_size - 1;
    strncpy(result, last_slash, result_len);
    result[result_len] = '\0';
}

void extract_path(const char* full_path, char* result, size_t result_size) {
    size_t len = strlen(full_path);

    const char* last_slash = strrchr(full_path, '/');
    const char* last_backslash = strrchr(full_path, '\\');
    if (last_slash == NULL && last_backslash == NULL)
        result[0] = '\0';
    else {
        last_slash = (last_slash > last_backslash) ? last_slash : last_backslash;
        size_t result_len = last_slash - full_path;
        if (result_len >= result_size)
            result_len = result_size - 1;
        strncpy(result, full_path, result_len);
        result[result_len] = '\0';
    }
}

int process_files(Model** models, LPQBuilder* pLpqBuilder) {
    int count = pLpqBuilder->fileCount;
    int modelCount = 0;

    for (int iterator = 0; iterator < count; ++iterator) {
        uint8* file = pLpqBuilder->pLpqInfo[iterator].pFileName;
        size_t length = strlen(file);
        if (strcmp((char*)file + length - 4, ".png") != 0) {
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
                for (int i = 0; i < count; ++i) {
                    file = pLpqBuilder->pLpqInfo[i].pFileName;
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
                models[modelCount++] = model;
            }
        }
    }
    return modelCount;
}