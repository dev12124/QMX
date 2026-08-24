#ifndef UNICODE
#define UNICODE
#endif 
// --- Bibliotecas ---
#include <windows.h>
#include <d3d12.h>
#include "d3dx12.h"
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <string>
#pragma comment(lib, "d3dcompiler.lib")
// --- Estrutura de Vértice ---
struct Vertex {
    float position[3];
    float color[4];
};
// --- Parte de FPS e DeltaTime ---
// Variáveis de Tempo e FPS
LARGE_INTEGER g_timerFrequency;
LARGE_INTEGER g_lastTime;
float g_deltaTime = 0.0f;
UINT g_fpsFrameCount = 0;
float g_fpsTimeAccumulator = 0.0f;
float g_currentFPS = 0.0f;
// --- Ponteiros ---
IDXGIFactory4* g_factory = nullptr;
ID3D12Device*  g_device  = nullptr;
ID3D12CommandQueue* g_cmdQueue = nullptr;
IDXGISwapChain3* g_swapChain = nullptr;
ID3D12DescriptorHeap* g_rtvHeap = nullptr;
ID3D12Resource* g_renderTargets[2] = {nullptr, nullptr};
ID3D12CommandAllocator* g_cmdAllocator = nullptr;
ID3D12GraphicsCommandList* g_cmdList = nullptr;
UINT g_rtvDescriptorSize = 0;
ID3D12Fence* g_fence = nullptr;
UINT64 g_fenceValue = 0;
HANDLE g_fenceEvent = nullptr;
UINT g_frameIndex = 0;
ID3DBlob* g_vsBlob = nullptr;
ID3DBlob* g_psBlob = nullptr;
ID3D12RootSignature* g_rootSignature = nullptr;
ID3D12PipelineState* g_pipelineState = nullptr;
// Ponteiros do Vertex Buffer
ID3D12Resource* g_vertexBuffer = nullptr;
D3D12_VERTEX_BUFFER_VIEW g_vertexBufferView = {};
// --- Funções ---
// Função para processar imagens
__attribute__((always_inline)) LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
} 
// Função que carrega, compila e mostra Erros de Sintaxe caso o HLSL tenha algum problema
HRESULT CompileShaderFromFile(const WCHAR* filename, LPCSTR entryPoint, LPCSTR profile, ID3DBlob** blobOut) {
  ID3DBlob* errorBlob = nullptr;
  HRESULT hr = D3DCompileFromFile(
    filename,
    nullptr,
    D3D_COMPILE_STANDARD_FILE_INCLUDE,
    entryPoint,
    profile,
    D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES,
    0,
    blobOut,
    &errorBlob
  );
  if (FAILED(hr) && errorBlob) {
    OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    MessageBoxA(nullptr, (char*)errorBlob->GetBufferPointer(), "Erro de Compilacao HLSL", MB_OK | MB_ICONERROR);
    errorBlob->Release();
  }
  return hr;
}
// Função Principal
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) { 
// Registrando...
WNDCLASSEXW wcx = {};
wcx.cbSize = sizeof(WNDCLASSEXW);
wcx.style = CS_HREDRAW | CS_VREDRAW;
wcx.lpfnWndProc = WindowProc;
wcx.hInstance = hInstance;
wcx.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(101));
wcx.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(101));
wcx.lpszClassName = L"QMX_Engine_Class";
if (!RegisterClassExW(&wcx)) {
  MessageBoxW(nullptr, L"Falha ao registrar a classe da janela!", L"Erro QMX", MB_OK | MB_ICONERROR);
  return 0;
}
// Renderizar a Janelinha!
HWND hwnd = CreateWindowExW(
            0,
            L"QMX_Engine_Class",
            L"QMX Engine v0.1",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            1280, 720,
            nullptr, nullptr, hInstance, nullptr
);
if (hwnd == nullptr) {
    MessageBoxW(nullptr, L"Falha ao criar a janela!", L"Erro QMX", MB_OK | MB_ICONERROR);
    return 0;
}
// Para aparecer o ícone personalizado
HICON hIconLarge = (HICON)LoadImageW(hInstance, MAKEINTRESOURCE(101), IMAGE_ICON, 32, 32, LR_SHARED);
HICON hIconSmall = (HICON)LoadImageW(hInstance, MAKEINTRESOURCE(101), IMAGE_ICON, 16, 16, LR_SHARED);
SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIconLarge);
SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSmall);
// E finalmente, renderiza a Janelinha
ShowWindow(hwnd, nCmdShow);
// 1. Criar a Fábrica DXGI 
if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&g_factory)))) {
    MessageBoxW(nullptr, L"Falha no Passo 1: CreateDXGIFactory1", L"Erro QMX", MB_OK | MB_ICONERROR);
    return 0;
}
// 2. Criar o Dispositivo apontando para a Intel UHD
if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&g_device)))) {
    MessageBoxW(nullptr, L"Falha no Passo 2: D3D12CreateDevice", L"Erro QMX", MB_OK | MB_ICONERROR);
    return 0;
}
// 3. Configurar e criar a Fila de Comandos
D3D12_COMMAND_QUEUE_DESC queueDesc = {};
queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
if (FAILED(g_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&g_cmdQueue)))) {
    MessageBoxW(nullptr, L"Falha no Passo 3: CreateCommandQueue", L"Erro QMX", MB_OK | MB_ICONERROR);
    return 0;
}
// 4. Configurar a Swap Chain
DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
swapChainDesc.BufferCount = 2;
swapChainDesc.Width = 1280;
swapChainDesc.Height = 720;
swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
swapChainDesc.SampleDesc.Count = 1;
// Criamos uma interface temporária-base para receber o Objeto do Windows
IDXGISwapChain1* tmpSwapChain = nullptr;
if (FAILED(g_factory->CreateSwapChainForHwnd(g_cmdQueue, hwnd, &swapChainDesc, nullptr, nullptr, &tmpSwapChain))) {
  MessageBoxW(nullptr, L"Falha no Passo 4: CreateSwapChainForHwnd", L"Erro QMX", MB_OK | MB_ICONERROR);
  return 0;
}
// Fazemos o Molde
if (FAILED(tmpSwapChain->QueryInterface(IID_PPV_ARGS(&g_swapChain)))) {
  MessageBoxW(nullptr, L"Falha no Passo 4: QueryInterface SwapChain3", L"Erro QMX", MB_OK | MB_ICONERROR);
  return 0;
}
// Liberamos o Ponteiro
tmpSwapChain->Release();
// 5. Criar heap de Descritores para o RTV
D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
rtvHeapDesc.NumDescriptors = 2;
rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
if (FAILED(g_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&g_rtvHeap)))) {
  MessageBoxW(nullptr, L"Falha no Passo 5: CreateDescriptorHeap", L"Erro QMX", MB_OK | MB_ICONERROR);
  return 0;
}
g_rtvDescriptorSize = g_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
// 6. Criar as visões (RTV) para cada Buffer da Swap Chain
D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(g_rtvHeap->GetCPUDescriptorHandleForHeapStart());
for (UINT i = 0; i < 2; i++) {
  if (FAILED(g_swapChain->GetBuffer(i, IID_PPV_ARGS(&g_renderTargets[i])))) {
    MessageBoxW(nullptr, L"Falha no Passo 6: GetBuffer da SwapChain", L"Erro QMX", MB_OK | MB_ICONERROR);
    return 0;
  }
  g_device->CreateRenderTargetView(g_renderTargets[i], nullptr, rtvHandle);
  rtvHandle.ptr += g_rtvDescriptorSize;
}
// 7. Criar alocador e lista de comandos
if (FAILED(g_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_cmdAllocator)))) {
  MessageBoxW(nullptr, L"Falha no Passo 7: CreateCommandAllocator", L"Erro QMX", MB_OK | MB_ICONERROR);
  return 0;
}
if (FAILED(g_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_cmdAllocator, nullptr, IID_PPV_ARGS(&g_cmdList)))) {
  MessageBoxW(nullptr, L"Falha no Passo 7: CreateCommandList", L"Erro QMX", MB_OK | MB_ICONERROR);
  return 0;
}
g_cmdList->Close();  // Permanece fechada até entrar no loop
// 8. Criar Fence para controlar a Intel UHD
if (FAILED(g_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence)))) {
  MessageBoxW(nullptr, L"Falha no Passo 8: CreateFence", L"Erro QMX", MB_OK | MB_ICONERROR);
  return 0;
}
g_fenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
// 9. Compila em Tempo de Execução os Shaders
// Compila o Vertex Shader
if (FAILED(CompileShaderFromFile(L".\\shaders.hlsl", "VSMain", "vs_5_0", &g_vsBlob))) {
  MessageBoxW(nullptr, L"Falha no Passo 9: Nao foi possivel carregar ou compilar o Vertex Shader em .\\shaders.hlsl", L"Erro QMX", MB_OK | MB_ICONERROR);
  return 0;
}
// Compila o Pixel Shader
if (FAILED(CompileShaderFromFile(L".\\shaders.hlsl", "PSMain", "ps_5_0", &g_psBlob))) {
  MessageBoxW(nullptr, L"Falha no Passo 9: Nao foi possivel carregar ou compilar o Pixel Shader em .\\shaders.hlsl", L"Erro QMX", MB_OK | MB_ICONERROR);
  return 0;
}
// 10. Criar a Root Signature Vazia (por enquanto)
D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
rootSignatureDesc.NumParameters = 0;
rootSignatureDesc.pParameters = nullptr;
rootSignatureDesc.NumStaticSamplers = 0;
rootSignatureDesc.pStaticSamplers = nullptr;
rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
ID3DBlob* signature = nullptr;
ID3DBlob* error = nullptr;
if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error))) {
  if (error) error->Release();
  MessageBoxW(nullptr, L"Falha no Passo 10: D3D12SerializeRootSignature", L"Erro QMX", MB_OK | MB_ICONERROR);
  return 0;
}
if (FAILED(g_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&g_rootSignature)))) {
  signature->Release();
  MessageBoxW(nullptr, L"Falha no Passo 10: CreateRootSignature", L"Erro QMX", MB_OK | MB_ICONERROR);
  return 0;
}
// 11. Definir o Layout de Entrada correspondente ao HLSL 
D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
  { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
  { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
};
// 12. Criar o Pipeline State Object (PSO)
D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
psoDesc.pRootSignature = g_rootSignature;
psoDesc.VS = { g_vsBlob->GetBufferPointer(), g_vsBlob->GetBufferSize() };
psoDesc.PS = { g_psBlob->GetBufferPointer(), g_psBlob->GetBufferSize() };
// Configuração Padrão do Rasterizer (sem CD3DX12)
psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
psoDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
psoDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
psoDesc.RasterizerState.DepthClipEnable = TRUE;
psoDesc.RasterizerState.MultisampleEnable = FALSE;
psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
psoDesc.RasterizerState.ForcedSampleCount = 0;
psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
// Configuração Padrão do BlendState (sem CD3DX12)
psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
psoDesc.BlendState.IndependentBlendEnable = FALSE;
psoDesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
psoDesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
psoDesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
psoDesc.DepthStencilState.DepthEnable = FALSE;
psoDesc.DepthStencilState.StencilEnable = FALSE;
psoDesc.SampleMask = UINT_MAX;
psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
psoDesc.NumRenderTargets = 1;
psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
psoDesc.SampleDesc.Count = 1;
if (FAILED(g_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&g_pipelineState)))) {
  MessageBoxW(nullptr, L"Falha no Passo 12: CreateGraphicsPipelineState", L"Erro QMX", MB_OK | MB_ICONERROR);
  return 0;
}
// 13. Definir os vértices do Triângulo (NDC: -1.0 a 1.0)
Vertex triangleVertices[] = {
    { {  0.0f,   0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } }, // Topo (Vermelho)
    { {  0.5f,  -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } }, // Direita (Verde)
    { { -0.5f,  -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }  // Esquerda (Azul)
};
const UINT vertexBufferSize = sizeof(triangleVertices);
// Configuração do Heap de Upload
D3D12_HEAP_PROPERTIES heapProps = {};
heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
D3D12_RESOURCE_DESC bufferDesc = {};
bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
bufferDesc.Width = vertexBufferSize;
bufferDesc.Height = 1;
bufferDesc.DepthOrArraySize = 1;
bufferDesc.MipLevels = 1;
bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
bufferDesc.SampleDesc.Count = 1;
bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
// Cria o Recurso de Memória na GPU
if (FAILED(g_device->CreateCommittedResource(
    &heapProps,
    D3D12_HEAP_FLAG_NONE,
    &bufferDesc,
    D3D12_RESOURCE_STATE_GENERIC_READ,
    nullptr,
    IID_PPV_ARGS(&g_vertexBuffer)))) {
    MessageBoxW(nullptr, L"Falha no Passo 13: CreateCommittedResource do Vertex Buffer", L"Erro QMX", MB_OK | MB_ICONERROR);
    return 0;
}
// Copia os dados da CPU para a GPU via Map/Unmap
UINT8* pVertexDataBegin = nullptr;
D3D12_RANGE readRange = { 0, 0 };
g_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
g_vertexBuffer->Unmap(0, nullptr);
// Cria a Visão do Vertex Buffer (VBV)
g_vertexBufferView.BufferLocation = g_vertexBuffer->GetGPUVirtualAddress();
g_vertexBufferView.StrideInBytes = sizeof(Vertex);
g_vertexBufferView.SizeInBytes = vertexBufferSize;
// --- Inicializa a Frequência do Relógio do Sistema e o Tempo Inicial ---
QueryPerformanceFrequency(&g_timerFrequency);
QueryPerformanceCounter(&g_lastTime);
// Loop Principal
MSG msg = {};
  while (msg.message != WM_QUIT) {
    if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    } else {
      // --- CÁLCULO DE DELTATIME E FPS ---
      LARGE_INTEGER currentTime;
      QueryPerformanceCounter(&currentTime);
      // DeltaTime em Segundos (Ex. 0.166s para 60 FPS)
      g_deltaTime = static_cast<float>(currentTime.QuadPart - g_lastTime.QuadPart) / static_cast<float>(g_timerFrequency.QuadPart);
      g_lastTime = currentTime;
      // Acumulador de FPS
      g_fpsFrameCount++;
      g_fpsTimeAccumulator += g_deltaTime;
      // Atualiza o Título da Janela a cada 1 segundo com a média de FPS
      if (g_fpsTimeAccumulator >= 1.0f) {
        g_currentFPS = static_cast<float>(g_fpsFrameCount) / g_fpsTimeAccumulator;
        std::wstring windowTitle = L"QMX Engine v0.1 | FPS: " + std::to_wstring(static_cast<int>(g_currentFPS)) + L" | DeltaTime: " + std::to_wstring(g_deltaTime * 100.0f).substr(0, 5) + L" ms";
        SetWindowTextW(hwnd, windowTitle.c_str());
        g_fpsFrameCount = 0;
        g_fpsTimeAccumulator = 0.0f;
      }
      // --- RENDERIZAÇÃO DIRECTX 12 ---
      // Pega o índice do buffer atual da GPU
      g_frameIndex = g_swapChain->GetCurrentBackBufferIndex();
      // prepara a gravação de comandos
      g_cmdAllocator->Reset();
      g_cmdList->Reset(g_cmdAllocator, nullptr);
      // Transição: De PRESENT -> RENDER_TARGET (para a GPU desenhar)
      D3D12_RESOURCE_BARRIER barrier = {};
      barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      barrier.Transition.pResource = g_renderTargets[g_frameIndex];
      barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
      barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
      barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      g_cmdList->ResourceBarrier(1, &barrier);
      // Pega o Ponteiro do Buffer na Memória
      D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(g_rtvHeap->GetCPUDescriptorHandleForHeapStart());
      rtvHandle.ptr += g_frameIndex * g_rtvDescriptorSize;
      // Limpa a Tela com a Cor Selecionada (RGBA: Roxo Escuro)
      const FLOAT clearColor[] = { 0.1f, 0.05f, 0.2f, 1.0f };
      g_cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
      g_cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
      // --- COMANDOS DE RENDERIZAÇÃO DO TRIÂNGULO ---
      D3D12_VIEWPORT viewport = { 0.0f, 0.0f, 1280.0f, 720.0f, 0.0f, 1.0f };
      D3D12_RECT scissorRect = { 0, 0, 1280, 720 };
      g_cmdList->RSSetViewports(1, &viewport);
      g_cmdList->RSSetScissorRects(1, &scissorRect);
      g_cmdList->SetGraphicsRootSignature(g_rootSignature);
      g_cmdList->SetPipelineState(g_pipelineState);
      g_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
      g_cmdList->IASetVertexBuffers(0, 1, &g_vertexBufferView);
      // Desenha o Triângulo
      g_cmdList->DrawInstanced(3, 1, 0, 0);
      // Transição: De RENDER_TARGET -> PRESENT (Para o Windows poder mostrar na Tela)
      barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
      barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
      g_cmdList->ResourceBarrier(1, &barrier);
      // Finaliza e manda a Lista para a Intel UHD
      g_cmdList->Close();
      ID3D12CommandList* ppCmdLists[] = { g_cmdList };
      g_cmdQueue->ExecuteCommandLists(1, ppCmdLists);
      // Exibe o Quadro na Tela com V-Sync ligado
      g_swapChain->Present(1, 0);
      // SINCRONIZAÇÃO (espera a GPU terminar antes de ir pro próximo quadro)
      const UINT64 fenceToWait = ++g_fenceValue;
      g_cmdQueue->Signal(g_fence, fenceToWait);
      if (g_fence->GetCompletedValue() < fenceToWait) {
        g_fence->SetEventOnCompletion(fenceToWait, g_fenceEvent);
        WaitForSingleObject(g_fenceEvent, INFINITE);
      }
    }
  }
  // Fim do QMX
  return 0;
}