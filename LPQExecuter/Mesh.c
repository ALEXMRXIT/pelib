/*
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

// Предположим, что структура Model и функция load_obj уже определены

typedef struct {
    IDirect3DVertexBuffer9* vertexBuffer;
    IDirect3DIndexBuffer9* indexBuffer;
    int vertexCount;
    int faceCount;
    IDirect3DTexture9* texture;
} RenderableModel;

RenderableModel* CreateRenderableModel(IDirect3DDevice9* device, Model* model, const char* textureFile) {
    if (!device || !model) return NULL;

    RenderableModel* renderableModel = (RenderableModel*)malloc(sizeof(RenderableModel));
    if (!renderableModel) return NULL;

    renderableModel->vertexCount = model->vertex_count;
    renderableModel->faceCount = model->face_count;

    // Создание вершинного буфера
    device->CreateVertexBuffer(
        model->vertex_count * sizeof(Vertex),
        D3DUSAGE_WRITEONLY,
        D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_NORMAL,
        D3DPOOL_DEFAULT,
        &renderableModel->vertexBuffer,
        NULL
    );

    // Заполнение вершинного буфера
    void* vertices;
    renderableModel->vertexBuffer->Lock(0, model->vertex_count * sizeof(Vertex), &vertices, 0);
    memcpy(vertices, model->vertices, model->vertex_count * sizeof(Vertex));
    renderableModel->vertexBuffer->Unlock();

    // Создание индексного буфера
    device->CreateIndexBuffer(
        model->face_count * 3 * sizeof(int),
        D3DUSAGE_WRITEONLY,
        D3DFMT_INDEX32,
        D3DPOOL_DEFAULT,
        &renderableModel->indexBuffer,
        NULL
    );

    // Заполнение индексного буфера
    void* indices;
    renderableModel->indexBuffer->Lock(0, model->face_count * 3 * sizeof(int), &indices, 0);
    int* indicesPtr = (int*)indices;
    for (int i = 0; i < model->face_count; i++) {
        indicesPtr[i * 3 + 0] = model->faces[i].v[0] - 1;
        indicesPtr[i * 3 + 1] = model->faces[i].v[1] - 1;
        indicesPtr[i * 3 + 2] = model->faces[i].v[2] - 1;
    }
    renderableModel->indexBuffer->Unlock();

    // Загрузка текстуры
    if (FAILED(D3DXCreateTextureFromFile(device, textureFile, &renderableModel->texture))) {
        MessageBox(NULL, "Failed to load texture", "Error!", MB_ICONEXCLAMATION | MB_OK);
        DestroyRenderableModel(renderableModel);
        return NULL;
    }

    return renderableModel;
}

void RenderModel(IDirect3DDevice9* device, RenderableModel* renderableModel) {
    if (!device || !renderableModel) return;

    // Установка буферов
    device->SetStreamSource(0, renderableModel->vertexBuffer, 0, sizeof(Vertex));
    device->SetIndices(renderableModel->indexBuffer);
    device->SetFVF(D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_NORMAL);

    // Установка текстуры
    device->SetTexture(0, renderableModel->texture);

    // Отрисовка модели
    device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, renderableModel->vertexCount, 0, renderableModel->faceCount);
}

void DestroyRenderableModel(RenderableModel* renderableModel) {
    if (renderableModel) {
        if (renderableModel->vertexBuffer) renderableModel->vertexBuffer->Release();
        if (renderableModel->indexBuffer) renderableModel->indexBuffer->Release();
        if (renderableModel->texture) renderableModel->texture->Release();
        free(renderableModel);
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Регистрация класса окна
    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "WindowClass";

    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // Создание окна
    HWND hwnd = CreateWindowEx(
        0,
        "WindowClass",
        "DirectX9 OBJ Loader",
        WS_EX_TOPMOST | WS_POPUP,
        0, 0, 800, 600,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) {
        MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Инициализация Direct3D
    IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d) {
        MessageBox(NULL, "Failed to create Direct3D interface", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = FALSE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.hDeviceWindow = hwnd;
    d3dpp.BackBufferWidth = 800;
    d3dpp.BackBufferHeight = 600;
    d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
    d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

    IDirect3DDevice9* device;
    if (FAILED(d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &device))) {
        MessageBox(NULL, "Failed to create Direct3D device", "Error!", MB_ICONEXCLAMATION | MB_OK);
        d3d->Release();
        return 0;
    }

    // Загрузка шейдера
    ID3DXEffect* effect;
    ID3DXBuffer* errorBuffer = NULL;
    if (FAILED(D3DXCreateEffectFromFile(device, "shader.fx", NULL, NULL, 0, NULL, &effect, &errorBuffer))) {
        if (errorBuffer) {
            MessageBox(NULL, (char*)errorBuffer->GetBufferPointer(), "Shader Error", MB_OK);
            errorBuffer->Release();
        }
        MessageBox(NULL, "Failed to create effect", "Error!", MB_ICONEXCLAMATION | MB_OK);
        device->Release();
        d3d->Release();
        return 0;
    }

    // Загрузка моделей
    const char* modelFiles[] = { "model1.obj", "model2.obj", "model3.obj" };
    const char* textureFiles[] = { "texture1.png", "texture2.png", "texture3.png" };
    int modelCount = sizeof(modelFiles) / sizeof(modelFiles[0]);
    RenderableModel** renderableModels = (RenderableModel**)malloc(modelCount * sizeof(RenderableModel*));

    for (int i = 0; i < modelCount; i++) {
        Model* model = load_obj(modelFiles[i]);
        if (!model) {
            MessageBox(NULL, "Failed to load model", "Error!", MB_ICONEXCLAMATION | MB_OK);
            for (int j = 0; j < i; j++) {
                DestroyRenderableModel(renderableModels[j]);
            }
            free(renderableModels);
            effect->Release();
            device->Release();
            d3d->Release();
            return 0;
        }

        renderableModels[i] = CreateRenderableModel(device, model, textureFiles[i]);
        if (!renderableModels[i]) {
            MessageBox(NULL, "Failed to create renderable model", "Error!", MB_ICONEXCLAMATION | MB_OK);
            for (int j = 0; j < i; j++) {
                DestroyRenderableModel(renderableModels[j]);
            }
            free(renderableModels);
            effect->Release();
            device->Release();
            d3d->Release();
            return 0;
        }

        free(model->vertices);
        free(model->faces);
        free(model);
    }

    // Основной цикл рендеринга
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            // Очистка экрана
            device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
            device->BeginScene();

            // Установка шейдера
            effect->SetTechnique("DefaultTechnique");
            UINT numPasses;
            effect->Begin(&numPasses, 0);

            for (UINT pass = 0; pass < numPasses; pass++) {
                effect->BeginPass(pass);

                // Отрисовка моделей
                for (int i = 0; i < modelCount; i++) {
                    RenderModel(device, renderableModels[i]);
                }

                effect->EndPass();
            }
            effect->End();

            device->EndScene();
            device->Present(NULL, NULL, NULL, NULL);
        }
    }

    // Освобождение ресурсов
    for (int i = 0; i < modelCount; i++) {
        DestroyRenderableModel(renderableModels[i]);
    }
    free(renderableModels);
    effect->Release();
    device->Release();
    d3d->Release();

    return (int)msg.wParam;
}

// shader.fx

// Структура вершинного шейдера
struct VS_OUTPUT {
    float4 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
};

// Вершинный шейдер
VS_OUTPUT VS(float4 Position : POSITION, float2 TexCoord : TEXCOORD0) {
    VS_OUTPUT output;
    output.Position = Position;
    output.TexCoord = TexCoord;
    return output;
}

// Пиксельный шейдер
float4 PS(VS_OUTPUT input) : COLOR {
    return tex2D(TextureSampler, input.TexCoord);
}

// Техника
technique DefaultTechnique {
    pass P0 {
        VertexShader = compile vs_2_0 VS();
        PixelShader = compile ps_2_0 PS();
    }
}

// Объявление текстуры и сэмплера
texture Texture;

sampler TextureSampler = sampler_state {
    Texture = <Texture>;
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = Linear;
    AddressU = Wrap;
    AddressV = Wrap;
};
*/

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