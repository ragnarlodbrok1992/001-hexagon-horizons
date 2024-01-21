#include "main.h"

// Engine defines
#define _DEBUG


// Windows required stuff
HWND m_hwnd = NULL;
RECT g_WindowRect;

// Engine variables
const uint8_t g_NumFrames = 3; // Triple buffering
bool g_UseWarp = false; // Use WARP adapter (software rendering)
bool g_IsInitialized = false; // Is the engine initialized?
int g_ScreenWidth = 1280;
int g_ScreenHeight = 720;

// DirectX 12 objects
Microsoft::WRL::ComPtr<ID3D12Device> g_Device;
Microsoft::WRL::ComPtr<ID3D12CommandQueue> g_CommandQueue;
Microsoft::WRL::ComPtr<IDXGISwapChain3> g_SwapChain;
Microsoft::WRL::ComPtr<ID3D12Resource> g_BackBuffers[g_NumFrames];
Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> g_CommandList;
Microsoft::WRL::ComPtr<ID3D12CommandAllocator> g_CommandAllocators[g_NumFrames];
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> g_RTVDescriptorHeap;

UINT g_RTVDescriptorSize;
UINT g_CurrentBackBufferIndex;

// Synchronization objects
Microsoft::WRL::ComPtr<ID3D12Fence> g_Fence;
uint64_t g_FenceValue = 0;
uint64_t g_FrameFenceValues[g_NumFrames] = {};
HANDLE g_FenceEvent;

// Controlling the swap chain present method
bool g_VSync = true;
bool g_TearingSupported = false;
bool g_Fullscreen = false;


// Window callback function
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

  switch (uMsg) {
    case WM_DESTROY: {
      PostQuitMessage(0);
      return 0;
    }
  }

  // Default message handler
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Create window function from tutorial
HWND EngineCreateWindow(const wchar_t* windowClassName, HINSTANCE hInst, const wchar_t* windowTitle, uint32_t width, uint32_t height) {
  int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
  int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);

  RECT windowRect = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
  ::AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

  int windowWidth = windowRect.right - windowRect.left;
  int windowHeight = windowRect.bottom - windowRect.top;

  int windowX = std::max<int>(0, (screenWidth - windowWidth) / 2);
  int windowY = std::max<int>(0, (screenHeight - windowHeight) / 2);

  HWND hWnd = ::CreateWindowExW(
      NULL,
      windowClassName,
      windowTitle,
      WS_OVERLAPPEDWINDOW,
      windowX,
      windowY,
      windowWidth,
      windowHeight,
      NULL,
      NULL,
      hInst,
      NULL
  );

  assert(hWnd && "Failed to create window");

  return hWnd;

}

// Getting adapter from tutorial
Microsoft::WRL::ComPtr<IDXGIAdapter4> GetAdapter(bool useWarp) {
  Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
  UINT createFactoryFlags = 0;
#if defined(_DEBUG)
  createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

  ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));
  Microsoft::WRL::ComPtr<IDXGIAdapter1> dxgiAdapter1;
  Microsoft::WRL::ComPtr<IDXGIAdapter4> dxgiAdapter4;

  if (useWarp) {
    ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1)));
    ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
  }
  else
  {
    SIZE_T maxDedicatedVideoMemory = 0;
    for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i) {
      DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
      dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

      if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
          SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)) && dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory) {
        maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
        ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
      }
    }
  }

  return dxgiAdapter4;
}

// Create device from tutorial
Microsoft::WRL::ComPtr<ID3D12Device2> CreateDevice(Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter) {
  Microsoft::WRL::ComPtr<ID3D12Device2> d3d12Device2;
  ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device2)));
  
#if defined(_DEBUG)
  Microsoft::WRL::ComPtr<ID3D12InfoQueue> pInfoQueue;
  if (SUCCEEDED(d3d12Device2.As(&pInfoQueue))) {
    pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
    pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
    pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

    D3D12_MESSAGE_SEVERITY severities[] = {
      D3D12_MESSAGE_SEVERITY_INFO
    };

    D3D12_MESSAGE_ID denyIds[] = {
      D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
      D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
      D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
    };

    D3D12_INFO_QUEUE_FILTER newFilter = {};
    newFilter.DenyList.NumSeverities = _countof(severities);
    newFilter.DenyList.pSeverityList = severities;
    newFilter.DenyList.NumIDs = _countof(denyIds);
    newFilter.DenyList.pIDList = denyIds;

    ThrowIfFailed(pInfoQueue->PushStorageFilter(&newFilter));
  }
#endif

  return d3d12Device2;
}

// Create command queue from tutorial
Microsoft::WRL::ComPtr<ID3D12CommandQueue> CreateCommandQueue(Microsoft::WRL::ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type) {
  Microsoft::WRL::ComPtr<ID3D12CommandQueue> d3d12CommandQueue;

  D3D12_COMMAND_QUEUE_DESC desc = {};
  desc.Type     = type;
  desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
  desc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
  desc.NodeMask = 0;

  ThrowIfFailed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d12CommandQueue)));

  return d3d12CommandQueue;
}

// Check tearing support
bool CheckTearingSupport() {
  BOOL allowTearing = FALSE;

  Microsoft::WRL::ComPtr<IDXGIFactory4> factory;
  if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
    Microsoft::WRL::ComPtr<IDXGIFactory5> factory5;
    if (SUCCEEDED(factory.As(&factory5))) {
      if (FAILED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing)))) {
        allowTearing = FALSE;
      }
    }
  }

  return allowTearing == TRUE;
}

// Create swap chain from tutorial
Microsoft::WRL::ComPtr<IDXGISwapChain4> CreateSwapChain(HWND hWnd, Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue, uint32_t width, uint32_t height, uint32_t bufferCount) {
  Microsoft::WRL::ComPtr<IDXGISwapChain4> dxgiSwapChain4;
  Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory4;

  UINT createFactoryFlags = 0;
#if defined(_DEBUG)
  createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

  ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));

  DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
  swapChainDesc.Width = width;
  swapChainDesc.Height = height;
  swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swapChainDesc.Stereo = FALSE;
  swapChainDesc.SampleDesc = {1, 0};
  swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapChainDesc.BufferCount = bufferCount;
  swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
  swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
  // It is recommended to always allow tearing if tearing support is available.
  swapChainDesc.Flags = CheckTearingSupport() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

  Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain1;
  ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(
      command_queue.Get(),
      hWnd,
      &swapChainDesc,
      nullptr,
      nullptr,
      &swapChain1
  ));

  // Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
  // will be handled manually.
  ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));

  ThrowIfFailed(swapChain1.As(&dxgiSwapChain4));

  return dxgiSwapChain4;
}

// Creating descriptor heaps from tutorial
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(Microsoft::WRL::ComPtr<ID3D12Device2> device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors) {
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;

  D3D12_DESCRIPTOR_HEAP_DESC desc = {};
  desc.NumDescriptors = numDescriptors;
  desc.Type = type;

  ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));

  return descriptorHeap;
}

// Creating render target views from tutorial
void UpdateRenderTargetViews(Microsoft::WRL::ComPtr<ID3D12Device2> device, Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap) {
  auto rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

  CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(descriptorHeap->GetCPUDescriptorHandleForHeapStart());

  for (int i = 0; i < g_NumFrames; ++i) {
    Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer;
    ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

    device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);

    g_BackBuffers[i] = backBuffer;

    rtvHandle.Offset(rtvDescriptorSize);
  }
}

// Create command allocator from tutorial
Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(Microsoft::WRL::ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type) {

  Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
  ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator)));

  return commandAllocator;
}

// Create command list from tutorial
Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CreateCommandList(Microsoft::WRL::ComPtr<ID3D12Device2> device, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type) {
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
  ThrowIfFailed(device->CreateCommandList(0, type, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

  ThrowIfFailed(commandList->Close());

  return commandList;
}

// Create fence from tutorial
Microsoft::WRL::ComPtr<ID3D12Fence> CreateFence(Microsoft::WRL::ComPtr<ID3D12Device2> device) {
  Microsoft::WRL::ComPtr<ID3D12Fence> fence;

  ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

  return fence;
}

// Debug function from tutorial
void EnableDebugLayer() {
#if defined(_DEBUG)
  Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface;
  ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface))); // This is tutorial exception stuff, we will get rid of this later since we do not plan to use exceptions, they are too runtime heavy
  debugInterface->EnableDebugLayer();
#endif
}

// Command line parsing function - from tutorial
void ParseCommandLineArguments() {
  int argc;
  wchar_t **argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);

  for (size_t i = 0; i < argc; ++i) {
    if (::wcscmp(argv[i], L"-w") == 0 || ::wcscmp(argv[i], L"--width") == 0) {
      g_ScreenWidth = ::wcstol(argv[i + 1], nullptr, 10);
    }
    if (::wcscmp(argv[i], L"-h") == 0 || ::wcscmp(argv[i], L"--height") == 0) {
      g_ScreenHeight = ::wcstol(argv[i + 1], nullptr, 10);
    }
    if (::wcscmp(argv[i], L"-warp") == 0 || ::wcscmp(argv[i], L"--warp") == 0) {
      g_UseWarp = true;
    }
  }

  // Free memory allocated by CommandLineToArgvW
  ::LocalFree(argv);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  // Parse command line arguments
  ParseCommandLineArguments();

  // Intialize window class
  WNDCLASSEX windowClass = {0};

  windowClass.cbSize = sizeof(WNDCLASSEX);
  windowClass.style = CS_HREDRAW | CS_VREDRAW;
  windowClass.lpfnWndProc = WindowProc;
  windowClass.cbClsExtra = 0;
  windowClass.cbWndExtra = 0;
  windowClass.hInstance = hInstance;
  windowClass.hIcon = ::LoadIcon(hInstance, NULL);
  windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
  windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  windowClass.lpszMenuName = NULL;
  windowClass.lpszClassName = "WindowClass";
  windowClass.hIconSm = ::LoadIcon(hInstance, NULL);

  static ATOM atom = ::RegisterClassEx(&windowClass);
  assert(atom > 0);

  // Create pSample which is DXSample

  // Some stuff with window rect? Using pSample which is DXSample
  // RECT windowRect = {0, 0, pSample.m_width, pSample.m_height};


  // Create the window nad store a handle to it
  /*
  m_hwnd = CreateWindow(
    windowClass.lpszClassName,
    "Hexagon Horizons",
    WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    g_ScreenWidth,
    g_ScreenHeight,
    nullptr,
    nullptr,
    hInstance,
    0);
    */

  // Show window
  // ShowWindow(m_hwnd, nCmdShow);

  // Message loop
  /*
  MSG msg = {};
  while (msg.message != WM_QUIT) {
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
  */

  // Windowsy return
  // return static_cast<char>(msg.wParam);
  return 0;
}
