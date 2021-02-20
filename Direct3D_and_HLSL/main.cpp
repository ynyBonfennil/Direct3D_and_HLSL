#include <Windows.h>
#include <tchar.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>
#include <iostream>
#include <DirectXMath.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

const unsigned int windowWidth = 1920;
const unsigned int windowHeight = 1080;

using namespace DirectX;

HRESULT CreateDXGIFactory(IDXGIFactory6** dxgiFactory) {
#ifdef _DEBUG
	if (FAILED(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(dxgiFactory)))) {
		if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(dxgiFactory)))) {
			return E_FAIL;
		}
	}
#else
	if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(dxgiFactory)))) {
		return E_FAIL;
	}
#endif
	return S_OK;
}

HRESULT GetNVIDIADXGIAdapter(IDXGIFactory6* dxgiFactory, IDXGIAdapter** adapter) {
	// List up all the adapters
	std::vector <IDXGIAdapter*> adapters;
	IDXGIAdapter* tmpAdapter = nullptr;
	for (int i = 0; dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i) {
		adapters.push_back(tmpAdapter);
	}

	// find the 1st adapter whose name contains NVIDIA
	for (auto adpt : adapters) {
		DXGI_ADAPTER_DESC adapterDesc = {};
		adpt->GetDesc(&adapterDesc);

		std::wstring description = adapterDesc.Description;
		if (description.find(L"NVIDIA") != std::string::npos) {
			adpt = adpt;
			return S_OK;
		}
	}
	return E_FAIL;
}

void SetSwapchainDesc(DXGI_SWAP_CHAIN_DESC1* desc) {
	desc->Width = windowWidth;
	desc->Height = windowHeight;
	desc->Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc->Stereo = false;
	desc->SampleDesc.Count = 1;
	desc->SampleDesc.Quality = 0;
	desc->BufferUsage = DXGI_USAGE_BACK_BUFFER;
	desc->BufferCount = 2;
	desc->Scaling = DXGI_SCALING_STRETCH;
	desc->SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc->AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	desc->Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
}

void SetDescHeapDescAsTypeRTV(D3D12_DESCRIPTOR_HEAP_DESC* desc) {
	desc->Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	desc->NodeMask = 0;
	desc->NumDescriptors = 2;
	desc->Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
}

void SetCommandQueueDesc(D3D12_COMMAND_QUEUE_DESC* desc) {
	desc->Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;		// タイムアウトなし
	desc->NodeMask = 0;
	desc->Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;	// プライオリティの指定なし
	desc->Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
}

void SetHeapProperties(D3D12_HEAP_PROPERTIES* heapProp) {
	heapProp->Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProp->CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProp->MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
}

void SetVertResDesc(D3D12_RESOURCE_DESC* resDesc, XMFLOAT3* vertices) {
	resDesc->Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc->Width = sizeof(vertices);
	resDesc->Height = 1;
	resDesc->DepthOrArraySize = 1;
	resDesc->MipLevels = 1;
	resDesc->Format = DXGI_FORMAT_UNKNOWN;
	resDesc->SampleDesc.Count = 1;
	resDesc->Flags = D3D12_RESOURCE_FLAG_NONE;
	resDesc->Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
}

void SetGraphicPipelineStateDesc(
	D3D12_GRAPHICS_PIPELINE_STATE_DESC* gpipeline,
	ID3DBlob* vsBlob,
	ID3DBlob* psBlob,
	D3D12_RENDER_TARGET_BLEND_DESC* renderTargetBlendDesc,
	D3D12_INPUT_ELEMENT_DESC* inputLayout,
	size_t inputLayoutSize,
	ID3D12RootSignature* rootsignature
) {
	gpipeline->pRootSignature = nullptr;
	gpipeline->VS.pShaderBytecode = vsBlob->GetBufferPointer();
	gpipeline->VS.BytecodeLength = vsBlob->GetBufferSize();
	gpipeline->PS.pShaderBytecode = psBlob->GetBufferPointer();
	gpipeline->PS.BytecodeLength = psBlob->GetBufferSize();
	gpipeline->SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	gpipeline->BlendState.AlphaToCoverageEnable = false;
	gpipeline->BlendState.IndependentBlendEnable = false;
	gpipeline->BlendState.RenderTarget[0] = *renderTargetBlendDesc;
	gpipeline->RasterizerState.MultisampleEnable = false;
	gpipeline->RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	gpipeline->RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	gpipeline->RasterizerState.DepthClipEnable = true;
	gpipeline->RasterizerState.FrontCounterClockwise = false;
	gpipeline->RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	gpipeline->RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	gpipeline->RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	gpipeline->RasterizerState.AntialiasedLineEnable = false;
	gpipeline->RasterizerState.ForcedSampleCount = 0;
	gpipeline->RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	gpipeline->DepthStencilState.DepthEnable = false;
	gpipeline->DepthStencilState.StencilEnable = false;
	gpipeline->InputLayout.pInputElementDescs = inputLayout;
	gpipeline->InputLayout.NumElements = inputLayoutSize;
	gpipeline->IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
	gpipeline->PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	gpipeline->NumRenderTargets = 1;
	gpipeline->RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	gpipeline->SampleDesc.Count = 1;
	gpipeline->SampleDesc.Quality = 0;
	gpipeline->pRootSignature = rootsignature;

}

LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	if (msg == WM_DESTROY) {
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void EnableDebugLayer() {
	ID3D12Debug* debugLayer = nullptr;
	auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
	debugLayer->EnableDebugLayer();
	debugLayer->Release();
}

int main() {

	// create window
	WNDCLASSEX window = {};
	window.cbSize = sizeof(WNDCLASSEX);
	window.lpfnWndProc = (WNDPROC)WindowProcedure;
	window.lpszClassName = _T("Direct3D and HLSL");
	window.hInstance = GetModuleHandle(nullptr);
	RegisterClassEx(&window);

	RECT windowRect = { 0, 0, windowWidth, windowHeight };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, false);
	HWND hwnd = CreateWindow(
		window.lpszClassName,
		window.lpszClassName,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		nullptr,
		nullptr,
		window.hInstance,
		nullptr
	);

#ifdef _DEBUG
	EnableDebugLayer();
#endif
	HRESULT result = S_OK;

	// Create DXGI Factory and get adapter
	//IDXGIFactory6* dxgiFactory = CreateDXGIFactory();
	//if (dxgiFactory == nullptr) { return -1; }
	IDXGIFactory6* dxgiFactory = nullptr;
	IDXGIAdapter* adpt = nullptr;
	result = CreateDXGIFactory(&dxgiFactory);
	result = GetNVIDIADXGIAdapter(dxgiFactory, &adpt);

	// Create D3D12Device
	ID3D12Device* dev = nullptr;
	D3D_FEATURE_LEVEL featureLevel;
	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};
	for (auto l : levels) {
		if (D3D12CreateDevice(adpt, l, IID_PPV_ARGS(&dev)) == S_OK) {
			featureLevel = l;
			break;
		}
	}

	// Create Command List and Allocator
	// Command Allocator contains the command data, and Command List
	// is sort of an interface of Command Allocator.
	ID3D12CommandAllocator* cmdAllocator = nullptr;
	ID3D12GraphicsCommandList* cmdList = nullptr;
	result = dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator));
	result = dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocator, nullptr, IID_PPV_ARGS(&cmdList));

	// Create Command Queue
	ID3D12CommandQueue* cmdQueue = nullptr;
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	SetCommandQueueDesc(&cmdQueueDesc);
	result = dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&cmdQueue));

	// Create Swap Chain
	IDXGISwapChain4* swapchain = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	SetSwapchainDesc(&swapchainDesc);
	result = dxgiFactory->CreateSwapChainForHwnd(
		cmdQueue,
		hwnd,
		&swapchainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)&swapchain
	);

	// Create Descriptor Heap of Render Target View (rtv)
	ID3D12DescriptorHeap* descHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	SetDescHeapDescAsTypeRTV(&descHeapDesc);
	result = dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&descHeap));

	// Associate each descriptors (rtv) with swapchain buffers (back buffers)
	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	result = swapchain->GetDesc(&swcDesc);
	std::vector<ID3D12Resource*> renderTargets(swcDesc.BufferCount);
	D3D12_CPU_DESCRIPTOR_HANDLE descHandle = descHeap->GetCPUDescriptorHandleForHeapStart();
	for (size_t i = 0; i < swcDesc.BufferCount; ++i) {
		result = swapchain->GetBuffer(static_cast<UINT>(i), IID_PPV_ARGS(&renderTargets[i]));
		dev->CreateRenderTargetView(renderTargets[i], nullptr, descHandle);
		descHandle.ptr += dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// Create Fence
	ID3D12Fence* fence = nullptr;
	UINT64 fenceVal = 0;
	result = dev->CreateFence(fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

	// show window for a while
	ShowWindow(hwnd, SW_SHOW);

	// Vertices
	XMFLOAT3 vertices[] = {
		{-0.4f, -0.7f, 0.0f},
		{-0.4f, 0.7f, 0.0f},
		{0.4f, -0.7f, 0.0f},
		{0.4f, 0.7f, 0.0f},
	};

	// Create Vertex Buffers on GPU
	D3D12_HEAP_PROPERTIES heapProp = {};
	D3D12_RESOURCE_DESC vertResDesc = {};
	ID3D12Resource* vertBuffer = nullptr;
	SetHeapProperties(&heapProp);
	SetVertResDesc(&vertResDesc, vertices);
	result = dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&vertResDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertBuffer)
	);

	// Copy vertices data from CPU to GPU (vertBuffer)
	// ID3D12ResourceMap() can map GPU memory area to CPU memory area
	// so copying data to this CPU memory area can affect that of GPU
	XMFLOAT3* vertMap = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	result = vertBuffer->Map(0, nullptr, (void**)&vertMap);
	std::copy(std::begin(vertices), std::end(vertices), vertMap);
	vertBuffer->Unmap(0, nullptr);

	// Create Vertex Buffer View for the data on GPU memory
	vbView.BufferLocation = vertBuffer->GetGPUVirtualAddress();
	vbView.SizeInBytes = sizeof(vertices);
	vbView.StrideInBytes = sizeof(vertices[0]);

	// Indices
	unsigned short indices[] = {
		0, 1, 2,
		2, 1, 3,
	};

	ID3D12Resource* idxBuffer = nullptr;
	vertResDesc.Width = sizeof(indices);
	result = dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&vertResDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&idxBuffer)
	);

	unsigned short* mappedIdx = nullptr;
	idxBuffer->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(std::begin(indices), std::end(indices), mappedIdx);
	idxBuffer->Unmap(0, nullptr);

	D3D12_INDEX_BUFFER_VIEW ibView = {};
	ibView.BufferLocation = idxBuffer->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = sizeof(indices);

	// HLSL related
	ID3DBlob* vsBlob = nullptr;
	ID3DBlob* psBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;

	// Compile Vertex Shader
	result = D3DCompileFromFile(L"VertexShader.hlsl",
		nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicVS", "vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0, &vsBlob, &errorBlob);
	if (FAILED(result)) {
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
			::OutputDebugStringA("File Not Found.");
		}
		else {
			std::string errstr;
			errstr.resize(errorBlob->GetBufferSize());
			std::copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
			errstr += "\n";
			OutputDebugStringA(errstr.c_str());
		}
		exit(1);
	}

	// Compile Pixel Shader
	result = D3DCompileFromFile(L"PixelShader.hlsl",
		nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicPS", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0, &psBlob, &errorBlob);
	if (FAILED(result)) {
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
			::OutputDebugStringA("File Not Found.");
		}
		else {
			std::string errstr;
			errstr.resize(errorBlob->GetBufferSize());
			std::copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
			errstr += "\n";
			OutputDebugStringA(errstr.c_str());
		}
		exit(1);
	}

	// Create Input Element Descriptor
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
	};

	// Create Render Target Blend Descriptor
	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
	renderTargetBlendDesc.BlendEnable = false;
	renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	renderTargetBlendDesc.LogicOpEnable = false;

	// Create Root Signature
	ID3D12RootSignature* rootsignature = nullptr;
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	ID3DBlob* rootSigBlob = nullptr;
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &errorBlob);
	result = dev->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&rootsignature));
	rootSigBlob->Release();

	// Create Graphics Pipeline State
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};
	ID3D12PipelineState* _pipelinestate = nullptr;
	SetGraphicPipelineStateDesc(
		&gpipeline,
		vsBlob,
		psBlob,
		&renderTargetBlendDesc,
		inputLayout,
		_countof(inputLayout),
		rootsignature
	);
	result = dev->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(&_pipelinestate));

	// Create Viewport and Scissor Rectangle
	D3D12_VIEWPORT viewport = {};
	D3D12_RECT scissorrect = {};
	viewport.Width = windowWidth;
	viewport.Height = windowHeight;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MaxDepth = 1.0f;
	viewport.MinDepth = 0.0f;
	scissorrect.top = 0;
	scissorrect.left = 0;
	scissorrect.right = scissorrect.left + windowWidth;
	scissorrect.bottom = scissorrect.top + windowHeight;

	MSG msg = {};
	unsigned int frame = 0;
	while (true) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// stop while loop when closed
		if (msg.message == WM_QUIT) { break; }

		// use swapchain
		auto bbIdx = swapchain->GetCurrentBackBufferIndex();

		// Create Resource Barrier
		D3D12_RESOURCE_BARRIER resourceBarrier = {};
		resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		resourceBarrier.Transition.pResource = renderTargets[bbIdx];
		resourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		// Switch from Present State to Render Target State
		resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		cmdList->ResourceBarrier(1, &resourceBarrier);

		// Set Pipeline State
		cmdList->SetPipelineState(_pipelinestate);

		// Get Render Target View of the next frame
		// and set it as the one for drawing
		auto rtvHandle = descHeap->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += static_cast<ULONG_PTR>(bbIdx * dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
		cmdList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);

		// Clear Render Target View
		float r, g, b;
		r = (float)(0xff & frame >> 16) / 255.0f;
		g = (float)(0xff & frame >> 8) / 255.0f;
		b = (float)(0xff & frame >> 0) / 255.0f;
		float clearColor[] = { r, g, b, 1.0f };
		cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
		++frame;

		// Set Viewports, Scissor Rectangles, Root Signature
		cmdList->RSSetViewports(1, &viewport);
		cmdList->RSSetScissorRects(1, &scissorrect);
		cmdList->SetGraphicsRootSignature(rootsignature);

		// Draw Polygons
		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmdList->IASetVertexBuffers(0, 1, &vbView);
		cmdList->IASetIndexBuffer(&ibView);
		cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);

		// Switch from Render Target State to Present State
		resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		cmdList->ResourceBarrier(1, &resourceBarrier);

		// Finish Adding Commands
		cmdList->Close();

		// Run commands
		ID3D12CommandList* cmdlists[] = { cmdList };
		cmdQueue->ExecuteCommandLists(1, cmdlists);

		cmdQueue->Signal(fence, ++fenceVal);

		// wait until command execution is done
		if (fence->GetCompletedValue() != fenceVal) {
			auto event = CreateEvent(nullptr, false, false, nullptr);
			fence->SetEventOnCompletion(fenceVal, event);
			WaitForSingleObject(event, INFINITE);
			CloseHandle(event);
		}

		cmdAllocator->Reset();
		cmdList->Reset(cmdAllocator, nullptr);

		swapchain->Present(1, 0);
	}
	UnregisterClass(window.lpszClassName, window.hInstance);
	return 0;
}

