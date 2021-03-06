// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//  are changed infrequently
//

#pragma once

#include "cmakeconfig.h"
#include "platform.hpp"

#ifdef CMAKE_HAVE_TS_FILESYSTEM
#define HAVE_TS_FILESYSTEM 1
#else
#define HAVE_TS_FILESYSTEM 0
#endif

// Change these values to use different versions
#include <SDKDDKVer.h>

#pragma warning(push)
// warning C4091: 'typedef ': ignored on left of 'tagSTRUCT' when no variable is declared
// warning C4302: 'type cast' : truncation from 'LPCTSTR' to 'WORD'
// warning C4458: declaration of 'name' hides class member
// warning C4838: conversion from 'int' to 'UINT' requires a narrowing conversion
#pragma warning(disable: 4091 4302 4458 4838)
#include <atlbase.h>
#include <atlapp.h>

extern CAppModule _Module;

#include <atlwin.h>

#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>
#include <atlctrlw.h>
#include <atlscrl.h>

#include <gdiplus.h>
#pragma warning(pop)

#if defined _M_IX86
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

#pragma comment (lib, "gdiplus.lib")

template <typename T, size_t length>
size_t __countof(T (&)[length]){ return length; }
template <typename T, size_t length>
size_t __countof(const T (&)[length]){ return length; }
