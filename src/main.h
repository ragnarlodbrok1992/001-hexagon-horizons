#ifndef _H_MAIN
#define _H_MAIN

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// Ancient macros undefined
#ifdef max
#undef max
#endif

#define NOMINMAX

#include <windows.h>
#include <shellapi.h>
#include <wrl.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#include <d3dx12.h>

#include <algorithm>
#include <cassert>
#include <chrono>

#include "Helpers.h"

#endif // _H_MAIN
