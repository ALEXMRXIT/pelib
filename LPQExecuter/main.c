#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "LPQEngine.h"
#include "Mesh.h"

char* getRootPath(const char* executablePath) {
    char* path = _strdup(executablePath);
    if (path == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        return NULL;
    }
    char* lastSlash = strrchr(path, '/');
    if (lastSlash == NULL)
        lastSlash = strrchr(path, '\\');
    if (lastSlash != NULL) 
        *lastSlash = '\0';
    return path;
}

char* addFolderToPath(const char* rootPath, const char* folder) {
    int size = strlen(rootPath) + strlen(folder) + 2;
    char* newPath = (char*)malloc(size);
    if (newPath == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        return NULL;
    }

    strcpy(newPath, rootPath);
    strcat(newPath, "\\");
    strcat(newPath, folder);

    free(rootPath);
    return newPath;
}

int main(int argc, char* argv[]) {
    if (argc < 1) return 1;

    LPQEngine* engine = CreateEngine();
    if (engine == NULL) return 1;

    char* rootPath = getRootPath(argv[0]);
    if (rootPath == NULL) return 1;

    char* corePath = addFolderToPath(rootPath, "gamedata");

    AnalizeTreeFiles(&engine->pLpqBuilder, corePath);

    int count = engine->pLpqBuilder->fileCount;
    Model** models = (Model**)malloc(count * sizeof(Model*));
    int modelCount = process_files(models, engine->pLpqBuilder);

    for (int i = 0; i < modelCount; ++i) {
        Model* model = models[i];
        printf("Model %d: %d vertices, %d faces, %d textures\n", i, model->vertex_count, model->face_count, model->textureCount);
        for (int j = 0; j < model->textureCount; ++j) {
            printf("  Texture %d: %s\n", j, model->textures[j]);
        }
    }

    for (int i = 0; i < modelCount; i++) {
        char filename[256];
        snprintf(filename, sizeof(filename), "%s\\%s.model", models[i]->root, models[i]->fileName);
        if (save_model_binary(filename, models[i]))
            printf("Model %d saved to %s\n", i, filename);
        else
            printf("Failed to save model %d to %s\n", i, filename);
    }

    for (int i = 0; i < modelCount; i++) {
        Model* model = models[i];
        free(model->vertices);
        free(model->faces);
        for (int j = 0; j < model->textureCount; j++)
            free(model->textures[j]);
        free(model->textures);
        free(model->fileName);
        free(model->root);
        free(model);
    }
    free(models);
	
    int ch;
    do {
        ch = getchar();
    } while (ch != '\n');

	return 0;
}