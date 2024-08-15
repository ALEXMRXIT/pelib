#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "LPQEngine.h"
#include "Mesh.h"
#include "Path.h"

#define USE_MEMORY_LEAK_DETECTED
#include "Memory.h"

int main(int argc, char* argv[]) {
    if (argc < 1) return 1;
    atexit(check_memory_leaks);

    LPQEngine* engine = CreateEngine();
    if (engine == NULL) return 1;

    char* rootPath = getRootPath(argv[0]);
    char* corePath = addFolderToPath(rootPath, "gamedata");

    const char* ignoreFile[] = { ".model", ".bhl", ".lpq" };

    AnalizeTreeFiles(&engine->pLpqBuilder, corePath, ignoreFile, 3);

    int count = engine->pLpqBuilder->modelCount;
    for (int iterator = 0; iterator < count; ++iterator) {
        Model* model = NULL;
        load_model(&model, engine->pLpqBuilder[iterator].pLpqInfo, engine->pLpqBuilder->fileCount);
        debug_model(model);
        save_model(model);
        delete_model(model);
    }
	
    int ch;
    do {
        ch = getchar();
    } while (ch != '\n');

    delete_engine(&engine);
    free(corePath);

	return 0;
}