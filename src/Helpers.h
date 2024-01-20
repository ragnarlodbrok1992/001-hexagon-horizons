#ifndef _H_HELPERS
#define _H_HELPERS

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

inline void ThrowIfFailed(HRESULT hr)
{
  if (FAILED(hr))
  {
    throw std::exception();
  }
}


#endif // _H_HELPERS
