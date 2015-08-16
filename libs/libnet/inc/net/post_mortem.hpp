/*
 * Copyright (C) 2013 midnightBITS
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

#ifndef __POST_MORTEM_HPP__
#define __POST_MORTEM_HPP__

#include <thread>
#include <net/filesystem.hpp>
#ifdef WIN32
#	include <windows.h>
#   pragma warning(push)
    // warning C4091: 'typedef ': ignored on left of 'tagSTRUCT' when no variable is declared
#   pragma warning(disable: 4091)
#   include <DbgHelp.h>
#   pragma warning(pop)
#   pragma comment(lib, "dbghelp.lib")
#endif

namespace pm
{
#ifdef WIN32

    class Win32PostMortemSupport {
        static fs::path tempDir() {
            auto size = GetTempPath(0, nullptr);
            if (!size)
                return fs::current_path();

            ++size;
            std::unique_ptr<WCHAR[]> path{ new WCHAR[size] };
            if (!GetTempPath(size, path.get()))
                return fs::current_path();

            return path.get();
        }

        static std::wstring buildPath() {
            auto temp = tempDir();
            temp /= "MiniDump.dmp";
            temp.make_preferred();
            return temp.wstring();
        }

        static LPCWSTR dumpPath() {
            static std::wstring path = buildPath();
            return path.c_str();
        }

        static void Write(_EXCEPTION_POINTERS* ep) {
            auto mdt = static_cast<MINIDUMP_TYPE>(
                MiniDumpWithDataSegs |
                MiniDumpWithFullMemory |
                MiniDumpWithFullMemoryInfo |
                MiniDumpWithHandleData |
                MiniDumpWithThreadInfo |
                MiniDumpWithProcessThreadData |
                MiniDumpWithUnloadedModules
                );

            auto file = CreateFileW(dumpPath(), GENERIC_READ | GENERIC_WRITE,
                0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

            if (file && file != INVALID_HANDLE_VALUE) {
                wchar_t buffer[2048];
                MINIDUMP_EXCEPTION_INFORMATION mei{ GetCurrentThreadId(), ep, TRUE };
                if (!MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, mdt, ep ? &mei : nullptr, nullptr, nullptr)) {
                    auto err = GetLastError();
                    CloseHandle(file);
                    DeleteFileW(dumpPath());
                    if (!err) {
                        swprintf_s(buffer, L"An error occured, but memory was not dumped due to unknown reason.");
                    } else {
                        swprintf_s(buffer, L"An error occured, but memory was not dumped due to error %08x.", err);
                    }
                } else {
                    CloseHandle(file);
                    if (ep) {
                        swprintf_s(buffer, L"An error %08x occured at %p (thread %u), memory dumped to:\n\n%s",
                            ep->ExceptionRecord->ExceptionCode,
                            ep->ExceptionRecord->ExceptionAddress,
                            GetCurrentThreadId(),
                            dumpPath());
                    } else {
                        swprintf_s(buffer, L"An unkown error occured, memory dumped to:\n\n%s", dumpPath());
                    }
                }
                buffer[__countof(buffer) - 1] = 0;
                MessageBoxW(nullptr, buffer, L"This program perfomed illegal operation", MB_ICONINFORMATION);
            }
            TerminateProcess(GetCurrentProcess(), 1);
        }

    public:
        template <class Fn, class... Args>
        static void Run(Fn&& fn, Args&&... args) {
            (void)dumpPath(); // build file path now for any OOM later...
            _EXCEPTION_POINTERS* exception = nullptr;
            __try {
                fn(std::forward<Args>(args)...);
            }
            __except (exception = GetExceptionInformation(), EXCEPTION_EXECUTE_HANDLER) {
                Write(exception);
            }
        }
    };

    template <class Fn, class... Args>
    inline void PostMortemSupport(Fn&& fn, Args&&... args) {
        Win32PostMortemSupport::Run(std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

#else // !WIN32
    template <class Fn, class... Args>
    inline void PostMortemSupport(Fn&& fn, Args&&... args) {
        fn(std::forward<Args>(args)...);
    }
#endif // WIN32
    template <class Fn, class... Args>
    inline std::thread thread(Fn&& fn, Args&&... args)
    {
        return std::thread{ PostMortemSupport<Fn, Args...>, std::forward<Fn>(fn), std::forward<Args>(args)... };
    }
};

#define BEGIN_MSG_MAP_POSTMORTEM(theClass) \
public: \
	BOOL ProcessWindowMessage(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, \
		_In_ LPARAM lParam, _Inout_ LRESULT& lResult, _In_ DWORD dwMsgMapID = 0) \
	{ \
		BOOL retVal = FALSE; \
		pm::PostMortemSupport([&, this] () mutable { \
			auto innerLambda = [&, this] () mutable -> BOOL { \
				BOOL bHandled = TRUE; \
				(hWnd); \
				(uMsg); \
				(wParam); \
				(lParam); \
				(lResult); \
				(bHandled); \
				switch (dwMsgMapID) \
				{ \
				case 0:

#define END_MSG_MAP_POSTMORTEM() \
					break; \
				default: \
					ATLTRACE(static_cast<int>(ATL::atlTraceWindowing), 0, _T("Invalid message map ID (%i)\n"), dwMsgMapID); \
					ATLASSERT(FALSE); \
					break; \
				} \
				return FALSE; \
			}; \
			retVal = innerLambda(); \
		}); \
		return retVal; \
	}

#endif // __POST_MORTEM_HPP__
