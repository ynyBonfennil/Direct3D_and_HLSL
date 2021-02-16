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


IDXGIAdapter* GetNVIDIADXGIAdapter(IDXGIFactory6* dxgiFactory) {
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
			return adpt;
		}
	}
}

DXGI_SWAP_CHAIN_DESC1 CreateSwapchainDesc() {
	DXGI_SWAP_CHAIN_DESC1 desc = {};
	desc.Width = windowWidth;
	desc.Height = windowHeight;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.Stereo = false;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	desc.BufferCount = 2;
	desc.Scaling = DXGI_SCALING_STRETCH;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	return desc;
}

LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	if (msg == WM_DESTROY) {
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
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

	// Create D3D12Device
	ID3D12Device* dev = nullptr;
	IDXGIFactory6* dxgiFactory = nullptr;
	IDXGIAdapter* adpt = nullptr;
	D3D_FEATURE_LEVEL featureLevel;

	HRESULT result = S_OK;
	if (FAILED(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&dxgiFactory)))) {
		if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgiFactory)))) {
			return -1;
		}
	}

	adpt = GetNVIDIADXGIAdapter(dxgiFactory);
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
	ID3D12CommandAllocator* cmdAllocator = nullptr;
	ID3D12GraphicsCommandList* cmdList = nullptr;
	result = dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator));
	result = dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocator, nullptr, IID_PPV_ARGS(&cmdList));

	// Create COmmand Queue
	ID3D12CommandQueue* cmdQueue = nullptr;

	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;		// タイムアウトなし
	cmdQueueDesc.NodeMask = 0;
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;	// プライオリティの指定なし
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	result = dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&cmdQueue));

	// Create Swap Chain
	IDXGISwapChain4* swapchain = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = CreateSwapchainDesc();
	result = dxgiFactory->CreateSwapChainForHwnd(
		cmdQueue,
		hwnd,
		&swapchainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)&swapchain
	);

	// Create Descriptor Heap
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ID3D12DescriptorHeap* rtvHeaps = nullptr;
	result = dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps));

	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	result = swapchain->GetDesc(&swcDesc);
	std::vector<ID3D12Resource*> backBuffers(swcDesc.BufferCount);
	D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	for (size_t i = 0; i < swcDesc.BufferCount; ++i) {
		result = swapchain->GetBuffer(static_cast<UINT>(i), IID_PPV_ARGS(&backBuffers[i]));
		dev->CreateRenderTargetView(backBuffers[i], nullptr, handle);
		handle.ptr += dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}
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

		D3D12_RESOURCE_BARRIER BarrierDesc = {};
		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		BarrierDesc.Transition.pResource = backBuffers[bbIdx];
		BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		cmdList->ResourceBarrier(1, &BarrierDesc);

		auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += static_cast<ULONG_PTR>(bbIdx * dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
		cmdList->OMSetRenderTargets(1, &rtvH, false, nullptr);

		float clearColor[] = { 0.0f, 0.0f, 1.0f, 1.0f };
		cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		cmdList->ResourceBarrier(1, &BarrierDesc);

		cmdList->Close();

		// Run commands
		ID3D12CommandList* cmdlists[] = { cmdList };
		cmdQueue->ExecuteCommandLists(1, cmdlists);

		cmdQueue->Signal(fence, ++fenceVal);

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

