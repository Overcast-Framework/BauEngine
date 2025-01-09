#include "bepch.h"
#include "D3D12Renderer.h"
#include <fstream>
#include <codecvt>

std::wstring ConvertStringToWString(const std::string& str)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(str);
}

std::string ConvertWStringToString(const std::wstring& wstr)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(wstr);
}

// from https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-d3d12createdevice
void GetHardwareAdapter(IDXGIFactory4* pFactory, IDXGIAdapter1** ppAdapter)
{
    *ppAdapter = nullptr;
    for (UINT adapterIndex = 0; ; ++adapterIndex)
    {
        IDXGIAdapter1* pAdapter = nullptr;
        if (DXGI_ERROR_NOT_FOUND == pFactory->EnumAdapters1(adapterIndex, &pAdapter))
        {
            // No more adapters to enumerate.
            break;
        }

        // Check to see if the adapter supports Direct3D 12, but don't create the
        // actual device yet.
        if (SUCCEEDED(D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
        {
            *ppAdapter = pAdapter;
            return;
        }
        pAdapter->Release();
    }
}


void D3D12Renderer::Initialize(GLFWwindow* window, glm::vec2 size)
{
    m_Window = window;
    m_Viewport.TopLeftX = 0;
    m_Viewport.TopLeftY = 0;
    m_Viewport.Height = size.y;
    m_Viewport.Width = size.x;

    m_Scissor.left = 0;
    m_Scissor.top = 0;
    m_Scissor.right = size.x;
    m_Scissor.bottom = size.y;

    LoadPipeline(size);
    LoadAssets();
}

void D3D12Renderer::BeginFrame()
{
    PopulateCmdList(m_VertexBuffer);
}

void D3D12Renderer::EndFrame()
{
    DX_ASSERT(m_SwapChain->Present(1, 0));
    WaitForPreviousFrame(m_Fence);
}

void D3D12Renderer::RenderFrame()
{
    ID3D12CommandList* ppCommandLists[] = { m_CmdList.Get() };
    m_CmdQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
}

void D3D12Renderer::Destroy()
{
    WaitForPreviousFrame(m_Fence);
    CloseHandle(m_Fence.m_FenceEvent);
}

void D3D12Renderer::LoadPipeline(glm::vec2 size)
{
    #ifdef DEBUG
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
        }
    }
    #endif

    // genq: why does the tutorial just spoonfeed the code?

    ComPtr<IDXGIFactory4> factory;
    D3D12_ASSERT(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory)), "Failed to create DXGI factory");

    ComPtr<IDXGIAdapter1> hwAdapter;
    GetHardwareAdapter(factory.Get(), &hwAdapter);
    D3D12_ASSERT(D3D12CreateDevice(hwAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_Device)), "Failed to create a D3D12 device");

    DXGI_ADAPTER_DESC1 adapterDesc;
    hwAdapter->GetDesc1(&adapterDesc);

    std::wcout << L"\tRenderer: " << adapterDesc.Description << std::endl;
    std::wcout << L"\tDedicated VRAM: " << adapterDesc.DedicatedVideoMemory / (1024*1024) << " MB" << std::endl;

    D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
    cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    D3D12_ASSERT(m_Device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&m_CmdQueue)), "Failed to create a D3D12 command queue");

    if (!m_Window) {
        throw std::runtime_error("GLFW window is not initialized!");
    }
    HWND hwnd = glfwGetWin32Window(m_Window);
    if (!hwnd) {
        throw std::runtime_error("Failed to retrieve HWND from GLFW window!");
    }

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = FIF;
    swapChainDesc.Width = size.x;
    swapChainDesc.Height = size.y;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFSDesc = {};
    swapChainFSDesc.Windowed = TRUE;

    ComPtr<IDXGISwapChain1> swapChain;

    D3D12_ASSERT(factory->CreateSwapChainForHwnd(m_CmdQueue.Get(), hwnd, &swapChainDesc, &swapChainFSDesc, nullptr, &swapChain), "Failed to create a D3D12 swapchain");
    D3D12_ASSERT(swapChain.As(&m_SwapChain), "Failure to run 'As' with swapchain");

    D3D12_ASSERT(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER), "Could not make window association");

    m_FrameIndex = m_SwapChain->GetCurrentBackBufferIndex();

    {
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = FIF;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        D3D12_ASSERT(m_Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_RTVHeap)), "Failed to create an rtv descriptor heap");

        D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
        srvHeapDesc.NumDescriptors = 1;
        srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        D3D12_ASSERT(m_Device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_SRVHeap)), "Failed to create an srv descriptor heap");

        m_DescHandleIncrSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_RTVHeap->GetCPUDescriptorHandleForHeapStart());

        for (UINT n = 0; n < FIF; n++)
        {
            D3D12_ASSERT(m_SwapChain->GetBuffer(n, IID_PPV_ARGS(&m_RTVs[n])), "Unable to get swch buffer for frame index " + std::to_string(n));
            m_Device->CreateRenderTargetView(m_RTVs[n].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, m_DescHandleIncrSize);
        }
    }

    D3D12_ASSERT(m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CmdAlloc)), "Failed to create a command allocator");
}

ComPtr<ID3D12PipelineState> D3D12Renderer::CreateShaderPipeline(std::wstring vs, std::wstring ps, const D3D12_INPUT_ELEMENT_DESC *ieDescs)
{
    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> pixelShader;

    #if defined(_DEBUG)
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
    #else
        UINT compileFlags = 0;
    #endif

    D3D12_INPUT_ELEMENT_DESC descs[3] = {};

    for (int i = 0; i < 3; i++)
        descs[i] = ieDescs[i];

    ComPtr<ID3DBlob> errorBlob;

    auto vsHR = D3DCompileFromFile(vs.c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, &errorBlob);
    DX_ASSERT(vsHR);

    if (!SUCCEEDED(vsHR))
    {
        std::cout << "HLSL Shader Compilation Error: " << (char*)errorBlob->GetBufferPointer() << std::endl;
    }

    vsHR = D3DCompileFromFile(ps.c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr);

    DX_ASSERT(vsHR);

    if (!SUCCEEDED(vsHR))
    {
        std::cout << "HLSL Shader Compilation Error: " << (char*)errorBlob->GetBufferPointer() << std::endl;
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

    psoDesc.InputLayout = { descs, 3 };
    psoDesc.pRootSignature = m_RootSig.Get();
    psoDesc.VS = { reinterpret_cast<UINT8*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize() };
    psoDesc.PS = { reinterpret_cast<UINT8*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;

    ComPtr<ID3D12PipelineState> pipelineState;

    D3D12_ASSERT(m_Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)), "Failed to create graphics pipeline");
    return pipelineState;
}

D3D12Fence D3D12Renderer::CreateFence()
{
    D3D12Fence fence{};
    DX_ASSERT(m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence.m_Fence)));
    fence.m_FenceValue = 1;

    fence.m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (fence.m_FenceEvent == nullptr)
    {
        DX_ASSERT(HRESULT_FROM_WIN32(GetLastError()));
    }

    WaitForPreviousFrame(fence);
    return fence;
}

void D3D12Renderer::PopulateCmdList(D3D12Buffer buf)
{
    DX_ASSERT(m_CmdList->Reset(m_CmdAlloc.Get(), m_PSO.Get()));

    m_CmdList->SetGraphicsRootSignature(m_RootSig.Get());

    ID3D12DescriptorHeap* ppHeaps[] = { m_SRVHeap.Get() };
    m_CmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    m_CmdList->SetGraphicsRootDescriptorTable(0, m_SRVHeap->GetGPUDescriptorHandleForHeapStart());
    m_CmdList->RSSetViewports(1, &m_Viewport);
    m_CmdList->RSSetScissorRects(1, &m_Scissor);

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_RTVs[m_FrameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_CmdList->ResourceBarrier(1, &barrier);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_RTVHeap->GetCPUDescriptorHandleForHeapStart(), m_FrameIndex, m_DescHandleIncrSize);
    m_CmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_CmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    m_CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_CmdList->IASetVertexBuffers(0, 1, &buf.m_vertexBufferView);
    m_CmdList->DrawInstanced(3, 1, 0, 0);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_RTVs[m_FrameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    m_CmdList->ResourceBarrier(1, &barrier);

    DX_ASSERT(m_CmdList->Close());
}

std::vector<UINT8> GenerateTextureData(UINT TextureWidth, UINT TextureHeight, UINT TexturePixelSize)
{
    const UINT rowPitch = TextureWidth * TexturePixelSize;
    const UINT cellPitch = rowPitch / 8;        // The width of a cell in the checkerboard texture.
    const UINT cellHeight = TextureHeight / 8; // The height of a cell in the checkerboard texture.
    const UINT textureSize = rowPitch * TextureHeight;

    std::vector<UINT8> data(textureSize);
    UINT8* pData = &data[0];

    for (UINT n = 0; n < textureSize; n += TexturePixelSize)
    {
        UINT x = (n % rowPitch) / TexturePixelSize; // Column in texture
        UINT y = n / rowPitch;                     // Row in texture
        UINT i = x / cellPitch;                    // Cell column
        UINT j = y / cellHeight;                   // Cell row

        bool isWhite = (i + j) % 2 == 0;           // Alternating cells

        pData[n] = isWhite ? 0xFF : 0x00;          // R
        pData[n + 1] = isWhite ? 0x00 : 0x00;      // G
        pData[n + 2] = isWhite ? 0x00 : 0x00;      // B
        pData[n + 3] = 0xFF;                       // A
    }

    return data;
}

D3D12_STATIC_SAMPLER_DESC D3D12Renderer::CreateSampler()
{
    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    sampler.MipLODBias = 0;
    sampler.MaxAnisotropy = 0;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    sampler.MinLOD = 0.0f;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister = 0;
    sampler.RegisterSpace = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    return sampler;
}

ComPtr<ID3D12Resource> textureUploadHeap;
ComPtr<ID3D12Resource> D3D12Renderer::CreateTexture2D(std::vector<UINT8> texData, UINT width, UINT height, UINT pixelSize, DXGI_FORMAT format, std::wstring Name)
{
    BE_ASSERT(texData.size() == width * height * pixelSize, "shut up shut up shut up");

    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.MipLevels = 1;
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.Format = format;
    texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    texDesc.DepthOrArraySize = 1;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    ComPtr<ID3D12Resource> texture;

    DX_ASSERT(
        m_Device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &texDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&texture)
        )
    );

    DX_ASSERT(texture->SetName(Name.c_str()));

    const UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture.Get(), 0, 1);

    DX_ASSERT(m_Device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&textureUploadHeap)));

    D3D12_SUBRESOURCE_DATA sTexData = {};
    sTexData.pData = &texData[0];
    sTexData.RowPitch = width * pixelSize;
    sTexData.SlicePitch = sTexData.RowPitch * height;

    m_CmdList->Reset(m_CmdAlloc.Get(), nullptr);

    UpdateSubresources(m_CmdList.Get(), texture.Get(), textureUploadHeap.Get(), 0, 0, 1, &sTexData);
    m_CmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = texDesc.Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    m_Device->CreateShaderResourceView(texture.Get(), &srvDesc, m_SRVHeap->GetCPUDescriptorHandleForHeapStart());

    DX_ASSERT(m_CmdList->Close());
    ID3D12CommandList* ppCommandLists[] = { m_CmdList.Get() };
    m_CmdQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    WaitForPreviousFrame(m_Fence);

    return texture;
}


void D3D12Renderer::LoadAssets()
{
    {
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

        if (FAILED(m_Device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
        {
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

        CD3DX12_ROOT_PARAMETER1 rootParameters[1];
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);

        auto sampler = CreateSampler();

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        DX_ASSERT(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
        DX_ASSERT(m_Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_RootSig)));
    }

    const D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "SV_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    m_PSO = CreateShaderPipeline(L"res/shaders/shaders.hlsl", L"res/shaders/shaders.hlsl", inputElementDescs);

    DX_ASSERT(m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_CmdAlloc.Get(), m_PSO.Get(), IID_PPV_ARGS(&m_CmdList)));
    DX_ASSERT(m_CmdList->Close());

    BEVertex TriVertices[3]
    {
        {glm::vec3{0.0f, 0.5f, 0.0f}, glm::vec3{0.0f, 0.0f, -1.0f}, glm::vec2{0.5f, 1.0f}},
        {glm::vec3{0.5f, -0.5f, 0.0f}, glm::vec3{0.0f, 0.0f, -1.0f}, glm::vec2{1.0f, 0.0f}},
        {glm::vec3{-0.5f, -0.5f, 0.0f}, glm::vec3{0.0f, 0.0f, -1.0f}, glm::vec2{0.0f, 0.0f}},
    };

    m_VertexBuffer = CreateBuffer<BEVertex, 3>(TriVertices, D3D12BufferType::Vertex);

    m_Fence = CreateFence();

    auto texture = Texture2D("res/tex/brick.jpg");
    auto txData = texture.ConvertToD3D12();
    m_Texture = CreateTexture2D(txData, texture.Width, texture.Height, 4, DXGI_FORMAT_R8G8B8A8_UNORM, ConvertStringToWString(texture.Name));

    texture.Free();
    WaitForPreviousFrame(m_Fence);
}

void D3D12Renderer::WaitForPreviousFrame(D3D12Fence fence)
{
    const UINT64 ufence = fence.m_FenceValue;
    DX_ASSERT(m_CmdQueue->Signal(fence.m_Fence.Get(), ufence));
    fence.m_FenceValue++;

    if (fence.m_Fence->GetCompletedValue() < ufence)
    {
        DX_ASSERT(fence.m_Fence->SetEventOnCompletion(ufence, fence.m_FenceEvent));
        WaitForSingleObject(fence.m_FenceEvent, INFINITE);
    }

    m_FrameIndex = m_SwapChain->GetCurrentBackBufferIndex();
}
