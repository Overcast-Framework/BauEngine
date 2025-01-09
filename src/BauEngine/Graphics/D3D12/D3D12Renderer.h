#pragma once
#include <bepch.h>
#include <BauEngine/Graphics/Renderer.h>
#include <BauEngine/Core/Utils.h>
#include <wrl.h>

using namespace Microsoft::WRL;

struct D3D12Buffer
{
	ComPtr<ID3D12Resource> m_BufferResource;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
};

struct D3D12Fence
{
	ComPtr<ID3D12Fence> m_Fence;
	UINT64 m_FenceValue;
	HANDLE m_FenceEvent;
};

enum class D3D12BufferType
{
	Vertex,
	Index,
	Constant
};

class D3D12Renderer : public Renderer
{
public:
	void Initialize(GLFWwindow* window, glm::vec2 size);
	void BeginFrame();
	void EndFrame();
	void RenderFrame();
	void Destroy();
private:
	static const int FIF = 2; // Frames-In-Flight
	UINT m_FrameIndex = 0;
	UINT m_DescHandleIncrSize = 0;

	GLFWwindow* m_Window;

	ComPtr<ID3D12Device> m_Device;
	ComPtr<ID3D12CommandQueue> m_CmdQueue;
	ComPtr<ID3D12GraphicsCommandList> m_CmdList;
	ComPtr<ID3D12DescriptorHeap> m_RTVHeap;
	ComPtr<ID3D12DescriptorHeap> m_SRVHeap;
	ComPtr<ID3D12Resource> m_RTVs[FIF];
	ComPtr<ID3D12CommandAllocator> m_CmdAlloc;
	ComPtr<ID3D12RootSignature> m_RootSig;
	ComPtr<ID3D12Resource> m_Texture;
	ComPtr<IDXGISwapChain3> m_SwapChain;
	ComPtr<ID3D12PipelineState> m_PSO;

	D3D12Fence m_Fence;
	D3D12Buffer m_VertexBuffer;
	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_Scissor;

	void LoadPipeline(glm::vec2 size);
	void LoadAssets();
	void WaitForPreviousFrame(D3D12Fence fence);

	ComPtr<ID3D12PipelineState> CreateShaderPipeline(std::wstring vs, std::wstring ps, const D3D12_INPUT_ELEMENT_DESC* ieDescs);
	D3D12Fence CreateFence();
	void PopulateCmdList(D3D12Buffer buf);

	D3D12_STATIC_SAMPLER_DESC CreateSampler();
	ComPtr<ID3D12Resource> CreateTexture2D(std::vector<UINT8> texData, UINT width, UINT height, UINT pixelSize, DXGI_FORMAT format, std::wstring Name = L"Untitled Texture");

	template <typename T, std::size_t N>
	D3D12Buffer CreateBuffer(T data[N], D3D12BufferType type)
	{
		const unsigned int bufferSize = N * sizeof(T);

		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
		auto desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
		ComPtr<ID3D12Resource> buffer;
		DX_ASSERT(m_Device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr, IID_PPV_ARGS(&buffer)));


		/*CD3DX12_HEAP_PROPERTIES upHeapProps(D3D12_HEAP_TYPE_UPLOAD);
		auto upDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
		ComPtr<ID3D12Resource> upHeap;
		DX_ASSERT(m_Device->CreateCommittedResource(&upHeapProps, D3D12_HEAP_FLAG_NONE, &upDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&upHeap)));*/

		UINT8* pDataBegin;
		CD3DX12_RANGE readRange(0, 0);

		DX_ASSERT(buffer->Map(0, &readRange, reinterpret_cast<void**>(&pDataBegin)));
		memcpy(pDataBegin, data, bufferSize);
		buffer->Unmap(0, nullptr);

		/*		m_CmdList->CopyBufferRegion(buffer.Get(), 0, upHeap.Get(), 0, bufferSize);

		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition
		(
			buffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
		);

		m_CmdList->ResourceBarrier(1, &barrier);
*/

		D3D12Buffer bufferStruct{};

		if (type == D3D12BufferType::Vertex)
		{
			D3D12_VERTEX_BUFFER_VIEW view;
			view.BufferLocation = buffer->GetGPUVirtualAddress();
			view.StrideInBytes = sizeof(BEVertex);
			view.SizeInBytes = bufferSize;

			bufferStruct.m_vertexBufferView = view;
		}
		bufferStruct.m_BufferResource = buffer;

		return bufferStruct;
	}
};