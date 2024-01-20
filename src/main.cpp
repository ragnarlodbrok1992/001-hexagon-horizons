#include "main.h"


// Windows required stuff
HWND m_hwnd = NULL;
RECT g_WindowRect;

// Engine variables
const uint8_t g_NumFrames = 3; // Triple buffering
const bool g_UseWarp = false; // Use WARP adapter (software rendering)
const bool g_IsInitialized = false; // Is the engine initialized?
const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;

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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  // Parse command line arguments
  int argc;
  LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
  if (argv == NULL) {
    MessageBoxW(NULL, L"Unable to parse command line arguments", L"Error", MB_OK | MB_ICONERROR);
    return 1;
  }

  LocalFree(argv);

  // Intialize window class
  WNDCLASSEX windowClass = {0};
  windowClass.cbSize = sizeof(WNDCLASSEX);
  windowClass.style = CS_HREDRAW | CS_VREDRAW;
  windowClass.lpfnWndProc = WindowProc;
  windowClass.hInstance = hInstance;
  windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
  windowClass.lpszClassName = "WindowClass";
  RegisterClassEx(&windowClass);

  // Create pSample which is DXSample

  // Some stuff with window rect? Using pSample which is DXSample
  // RECT windowRect = {0, 0, pSample.m_width, pSample.m_height};


  // Create the window nad store a handle to it
  m_hwnd = CreateWindow(
    windowClass.lpszClassName,
    "Hexagon Horizons",
    WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    nullptr,
    nullptr,
    hInstance,
    0);

  // Show window
  ShowWindow(m_hwnd, nCmdShow);

  // Message loop
  MSG msg = {};
  while (msg.message != WM_QUIT) {
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  // Windowsy return
  return static_cast<char>(msg.wParam);
}
