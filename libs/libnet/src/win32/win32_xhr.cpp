/*
 * Copyright (C) 2014 midnightBITS
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

#include "pch.h"
#include <sdkddkver.h>
#include <windows.h>

namespace net { namespace http { namespace client {
	struct OSInfo {
		bool isNt;
		bool isWow64;
		struct version_t {
			DWORD major;
			DWORD minor;
		} version;

		OSInfo()
			: isNt(ProductIsNT())
			, isWow64(IsWow64Process(GetCurrentProcess()))
			, version(GetVersion())
		{
		}

		static bool IsWow64Process(HANDLE process_handle) {
			typedef BOOL(WINAPI* IsWow64ProcessFunc)(HANDLE, PBOOL);
			IsWow64ProcessFunc is_wow64_process = reinterpret_cast<IsWow64ProcessFunc>(
				GetProcAddress(GetModuleHandle(L"kernel32.dll"), "IsWow64Process"));
			if (!is_wow64_process)
				return false;
			BOOL is_wow64 = FALSE;
			if (!(*is_wow64_process)(process_handle, &is_wow64))
				return false;
			return is_wow64 != FALSE;
		}

		static bool IsWindowsVersionOrGreater(WORD wMajorVersion, WORD wMinorVersion = (WORD)-1)
		{
			OSVERSIONINFOEX osvi = { sizeof(osvi), 0, 0, 0, 0, { 0 }, 0, 0 };
			DWORDLONG condition = 0;
			VER_SET_CONDITION(condition, VER_MAJORVERSION, VER_GREATER_EQUAL);
			DWORD mask = VER_MAJORVERSION;
			if (wMinorVersion != (WORD)-1) {
				VER_SET_CONDITION(condition, VER_MINORVERSION, VER_GREATER_EQUAL);
				mask |= VER_MINORVERSION;
			}

			osvi.dwMajorVersion = wMajorVersion;
			osvi.dwMinorVersion = wMinorVersion;

			return VerifyVersionInfo(&osvi, mask, condition) != FALSE;
		}

		static bool EqualsProductType(BYTE productType)
		{
			OSVERSIONINFOEX osvi = { sizeof(osvi), 0, 0, 0, 0, { 0 }, 0, 0 };
			ULONGLONG condition = VerSetConditionMask(0, VER_PRODUCT_TYPE, VER_EQUAL);
			osvi.wProductType = productType;

			return VerifyVersionInfo(&osvi, VER_PRODUCT_TYPE, condition) != FALSE;
		}

		bool ProductIsNT()
		{
			return (
				EqualsProductType(VER_NT_WORKSTATION) ||
				EqualsProductType(VER_NT_SERVER) ||
				EqualsProductType(VER_NT_DOMAIN_CONTROLLER)
				);
		}

		static version_t GetVersion()
		{
			WORD major = 5;
			for (; major < 100; ++major) {
				if (!IsWindowsVersionOrGreater(major)) {
					--major;
					break;
				}
			}

			WORD minor = 0;
			for (; minor < 100; ++minor) {
				if (!IsWindowsVersionOrGreater(major, minor)) {
					--minor;
					break;
				}
			}

			return{major, minor};
		}

	};

	std::string os_client_info()
	{
		static OSInfo info;
		std::string   version = info.isNt ? "Windows NT" : "Windows";
		version += "/";
		version += std::to_string(info.version.major);
		version += ".";
		version += std::to_string(info.version.major);
		if (info.isWow64)
			version += "; WOW64";
#ifdef UNICODE
		version += "; U";
#endif
		return version;
	}
}}}
