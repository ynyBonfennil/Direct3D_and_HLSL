#include <Windows.h>
#include <tchar.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>
#include <iostream>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

const unsigned int windowWidth = 1920;
const unsigned int windowHeight = 1080;


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

	MSG msg = {};
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

		// Get Render Target View of the next frame
		// and set it as the one for drawing
		auto rtvHandle = descHeap->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += static_cast<ULONG_PTR>(bbIdx * dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
		cmdList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);

		// Add Cmmand to Clear Render Target View to Command List
		float clearColor[] = { 0.0f, 0.0f, 1.0f, 1.0f };
		cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

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

