/*
 * Copyright (C) 2015 midnightBITS
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

 // The following macros define the minimum required platform.  The minimum required platform
 // is the earliest version of Windows, Internet Explorer etc. that has the necessary features to run 
 // your application.  The macros work by enabling all features available on platform versions up to and 
 // including the version specified.

 // Modify the following defines if you have to target a platform prior to the ones specified below.
 // Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER                  // Specifies that the minimum required platform is Windows 2000.
#define WINVER 0x0500           // Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINNT            // Specifies that the minimum required platform is Windows 2000.
#define _WIN32_WINNT WINVER     // Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_IE               // Specifies that the minimum required platform is Internet Explorer 5.0.
#define _WIN32_IE WINVER        // Change this to the appropriate value to target other versions of IE.
#endif

#ifndef _WIN32_MSI              // Specifies that the minimum required MSI version is MSI 3.1
#define _WIN32_MSI 310          // Change this to the appropriate value to target other versions of MSI.
#endif

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
 // Windows Header Files:
#include <windows.h>
#include <strsafe.h>
#include <msiquery.h>

 // WiX Header Files:
#include <wcautil.h>

#include <utility>

#undef ExitOnFailure
#define ExitOnFailure(x, s, ...)   if (FAILED(x)) { ExitTrace(x, s, __VA_ARGS__);  return x; }

template <typename Final>
struct ActionHandler {
	static UINT call(MSIHANDLE hInstall)
	{
		HRESULT hr = WcaInitialize(hInstall, Final::action_name());
		if (FAILED(hr))
			ExitTrace(hr, "Failed to initialize");
		else
			hr = Final::action();

		UINT er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
		return WcaFinalize(er);
	}

	template <typename... Args>
	static void log(__in LOGLEVEL llv, __in_z __format_string PCSTR fmt, Args&&... args)
	{
		WcaLog(llv, fmt, std::forward<Args>(args)...);
	}
};

#define CUSTOM_ACTION(name) \
struct name ## Handler : ActionHandler<name##Handler> { \
	static const char* action_name() { return #name; } \
	static HRESULT action(); \
}; \
extern "C" UINT __stdcall name(MSIHANDLE hInstall) { return name ## Handler::call(hInstall); } \
__pragma(comment(linker, "/EXPORT:" #name "=_" #name "@4")); \
HRESULT name ## Handler::action()
