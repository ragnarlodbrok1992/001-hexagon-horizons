#include "main.h"


HWND m_hwnd = NULL;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

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

  // Create the window nad store a handle to it
  m_hwnd = CreateWindow(
    windowClass.lpszClassName,
    "Test window",
    WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    800,
    600,
    nullptr,
    nullptr,
    hInstance,
    nullptr);

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
