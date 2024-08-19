#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Mesh.h>
#include <Memory.h>

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

typedef struct {
    IDirect3DVertexBuffer9* vertexBuffer;
    IDirect3DIndexBuffer9* indexBuffer;
    int vertexCount;
    int faceCount;
    IDirect3DTexture9* texture;
} RenderableModel;



RenderableModel* CreateRenderableModel(IDirect3DDevice9* device, Model* model, void* textureBuffer, size_t textureBufferSize) {
    if (!device || !model || !textureBuffer || textureBufferSize == 0) return NULL;

    RenderableModel* renderableModel = (RenderableModel*)malloc(sizeof(RenderableModel));
    if (!renderableModel) return NULL;

    renderableModel->vertexCount = model->vertex_count;
    renderableModel->faceCount = model->face_count;

    device->lpVtbl->CreateVertexBuffer(
        device,
        model->vertex_count * sizeof(Vertex),
        D3DUSAGE_WRITEONLY,
        D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_NORMAL,
        D3DPOOL_DEFAULT,
        &renderableModel->vertexBuffer,
        NULL
    );

    void* vertices;
    renderableModel->vertexBuffer->lpVtbl->Lock(renderableModel->vertexBuffer, 0, model->vertex_count * sizeof(Vertex), &vertices, 0);
    memcpy(vertices, model->vertices, model->vertex_count * sizeof(Vertex));
    renderableModel->vertexBuffer->lpVtbl->Unlock(renderableModel->vertexBuffer);

    device->lpVtbl->CreateIndexBuffer(
        device,
        model->face_count * 3 * sizeof(int),
        D3DUSAGE_WRITEONLY,
        D3DFMT_INDEX32,
        D3DPOOL_DEFAULT,
        &renderableModel->indexBuffer,
        NULL
    );

    void* indices;
    renderableModel->indexBuffer->lpVtbl->Lock(renderableModel->indexBuffer, 0, model->face_count * 3 * sizeof(int), &indices, 0);
    int* indicesPtr = (int*)indices;
    for (int i = 0; i < model->face_count; i++) {
        indicesPtr[i * 3 + 0] = model->faces[i].v[0] - 1;
        indicesPtr[i * 3 + 1] = model->faces[i].v[1] - 1;
        indicesPtr[i * 3 + 2] = model->faces[i].v[2] - 1;
    }
    renderableModel->indexBuffer->lpVtbl->Unlock(renderableModel->indexBuffer);

    if (FAILED(D3DXCreateTextureFromFileInMemory(device, textureBuffer, textureBufferSize, &renderableModel->texture))) {
        MessageBox(NULL, "Failed to load texture from buffer", "Error!", MB_ICONEXCLAMATION | MB_OK);
        DestroyRenderableModel(renderableModel);
        return NULL;
    }

    return renderableModel;
}

void RenderModel(IDirect3DDevice9* device, RenderableModel* renderableModel) {
    if (!device || !renderableModel) return;

    device->lpVtbl->SetStreamSource(device, 0, renderableModel->vertexBuffer, 0, sizeof(Vertex));
    device->lpVtbl->SetIndices(device, renderableModel->indexBuffer);
    device->lpVtbl->SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_NORMAL);

    device->lpVtbl->SetTexture(device, 0, renderableModel->texture);

    device->lpVtbl->DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, 0, 0, renderableModel->vertexCount, 0, renderableModel->faceCount);
}

void DestroyRenderableModel(RenderableModel* renderableModel) {
    if (renderableModel) {
        if (renderableModel->vertexBuffer)
            renderableModel->vertexBuffer->lpVtbl->Release(renderableModel->vertexBuffer);
        if (renderableModel->indexBuffer)
            renderableModel->indexBuffer->lpVtbl->Release(renderableModel->indexBuffer);
        if (renderableModel->texture)
            renderableModel->texture->lpVtbl->Release(renderableModel->texture);
        free(renderableModel);
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            PostQuitMessage(0);
        }
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
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
    if (FAILED(d3d->lpVtbl->CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &device))) {
        MessageBox(NULL, "Failed to create Direct3D device", "Error!", MB_ICONEXCLAMATION | MB_OK);
        d3d->lpVtbl->Release(d3d);
        return 0;
    }

    const char* shader =
        "struct VS_OUTPUT {\n"
        "    float4 Position : POSITION;\n"
        "    float2 TexCoord : TEXCOORD0;\n"
        "};\n"
        "\n"
        "VS_OUTPUT VS(float4 Position : POSITION, float2 TexCoord : TEXCOORD0) {\n"
        "    VS_OUTPUT output;\n"
        "    output.Position = Position;\n"
        "    output.TexCoord = TexCoord;\n"
        "    return output;\n"
        "}\n"
        "\n"
        "float4 PS(VS_OUTPUT input) : COLOR {\n"
        "    return tex2D(TextureSampler, input.TexCoord);\n"
        "}\n"
        "\n"
        "technique DefaultTechnique {\n"
        "    pass P0 {\n"
        "        VertexShader = compile vs_2_0 VS();\n"
        "        PixelShader = compile ps_2_0 PS();\n"
        "    }\n"
        "}\n"
        "\n"
        "texture Texture;\n"
        "\n"
        "sampler TextureSampler = sampler_state {\n"
        "    Texture = <Texture>;\n"
        "    MinFilter = Linear;\n"
        "    MagFilter = Linear;\n"
        "    MipFilter = Linear;\n"
        "    AddressU = Wrap;\n"
        "    AddressV = Wrap;\n"
        "};";
    ID3DXEffect* effect;
    ID3DXBuffer* errorBuffer = NULL;
    size_t shaderCodeSize = strlen(shader);
    if (FAILED(D3DXCreateEffectFromMemory(device, shader, shaderCodeSize, NULL, NULL, 0, NULL, &effect, &errorBuffer))) {
        if (errorBuffer) {
            MessageBox(NULL, (char*)errorBuffer->lpVtbl->GetBufferPointer(errorBuffer), "Shader Error", MB_OK);
            errorBuffer->lpVtbl->Release(errorBuffer);
        }
        MessageBox(NULL, "Failed to create effect from memory", "Error!", MB_ICONEXCLAMATION | MB_OK);
        device->lpVtbl->Release(device);
        d3d->lpVtbl->Release(d3d);
        return 0;
    }

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
            effect->lpVtbl->Release(effect);
            device->lpVtbl->Release(device);
            d3d->lpVtbl->Release(d3d);
            return 0;
        }

        size_t textureBufferSize;
        void* textureBuffer = LoadTextureToBuffer(textureFiles[i], &textureBufferSize);
        if (!textureBuffer) {
            MessageBox(NULL, "Failed to load texture to buffer", "Error!", MB_ICONEXCLAMATION | MB_OK);
            free(model->vertices);
            free(model->faces);
            free(model);
            for (int j = 0; j < i; j++) {
                DestroyRenderableModel(renderableModels[j]);
            }
            free(renderableModels);
            effect->lpVtbl->Release(effect);
            device->lpVtbl->Release(device);
            d3d->lpVtbl->Release(d3d);
            return 0;
        }

        renderableModels[i] = CreateRenderableModel(device, model, textureBuffer, textureBufferSize);
        if (!renderableModels[i]) {
            MessageBox(NULL, "Failed to create renderable model", "Error!", MB_ICONEXCLAMATION | MB_OK);
            FreeTextureBuffer(textureBuffer);
            free(model->vertices);
            free(model->faces);
            free(model);
            for (int j = 0; j < i; j++) {
                DestroyRenderableModel(renderableModels[j]);
            }
            free(renderableModels);
            effect->lpVtbl->Release(effect);
            device->lpVtbl->Release(device);
            d3d->lpVtbl->Release(d3d);
            return 0;
        }

        FreeTextureBuffer(textureBuffer);
        free(model->vertices);
        free(model->faces);
        free(model);
    }

    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            device->lpVtbl->Clear(device, 0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
            device->lpVtbl->BeginScene(device);

            effect->lpVtbl->SetTechnique(effect, "DefaultTechnique");
            UINT numPasses;
            effect->lpVtbl->Begin(effect, &numPasses, 0);

            for (UINT pass = 0; pass < numPasses; pass++) {
                effect->lpVtbl->BeginPass(effect, pass);

                for (int i = 0; i < modelCount; i++) {
                    RenderModel(device, renderableModels[i]);
                }

                effect->lpVtbl->EndPass(effect);
            }
            effect->lpVtbl->End(effect);

            device->lpVtbl->EndScene(device);
            device->lpVtbl->Present(device, NULL, NULL, NULL, NULL);
        }
    }

    for (int i = 0; i < modelCount; i++) {
        DestroyRenderableModel(renderableModels[i]);
    }
    free(renderableModels);
    effect->lpVtbl->Release(effect);
    device->lpVtbl->Release(device);
    d3d->lpVtbl->Release(d3d);

    return (int)msg.wParam;
}