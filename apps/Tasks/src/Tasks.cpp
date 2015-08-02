// [!output PROJECT_NAME].cpp : main source file for [!output PROJECT_NAME].exe
//

#include "stdafx.h"

#include "resource.h"
#include "version.h"

#include "TasksView.h"
#include "AboutDlg.h"
#include "TasksFrame.h"
#include "langs.h"

#include <shellapi.h>

#include <memory>
#include <net/filesystem.hpp>
#include <net/xhr.hpp>
#include <net/post_mortem.hpp>

#include <gdiplus.h>

namespace fs = filesystem;

#define WIDE2(x) L ## x
#define WIDE(x) WIDE2(x)

#define WPROGRAM_NAME WIDE(PROGRAM_NAME)
#define WPROGRAM_VERSION_STRING WIDE(PROGRAM_VERSION_STRING)
#define WPROGRAM_VERSION_STABILITY WIDE(PROGRAM_VERSION_STABILITY)
#define WPROGRAM_VERSION_BUILD WIDE(VERSION_STRINGIFY(PROGRAM_VERSION_BUILD))

#ifndef BULID_NUMBER_IN_TITLE
#define BULID_NUMBER_IN_TITLE 1
#endif

fs::path exe_dir() {
	static fs::path dir = fs::app_directory();
	return dir;
}
namespace dpi {
	enum class aware {
		unaware = 0,
		system = 1,
		monitor = 2
	};

	typedef enum PROCESS_DPI_AWARENESS {
		PROCESS_DPI_UNAWARE = 0,
		PROCESS_SYSTEM_DPI_AWARE = 1,
		PROCESS_PER_MONITOR_DPI_AWARE = 2
	} PROCESS_DPI_AWARENESS;

	aware checkHighDpi()
	{
		aware result = aware::unaware;

		HMODULE hShcore = LoadLibrary(_T("shcore.dll"));

		if (hShcore) {
			typedef HRESULT(STDAPICALLTYPE *SetProcessDpiAwarenessFunc)(_In_ PROCESS_DPI_AWARENESS value);
			auto setProcessDpiAwareness = (SetProcessDpiAwarenessFunc)GetProcAddress(hShcore, "SetProcessDpiAwareness");
			if (setProcessDpiAwareness) {
				HRESULT hr = setProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE);
				if (SUCCEEDED(hr))
					result = aware::system;
				else if (hr == E_ACCESSDENIED)
					result = aware::system; // TODO: read from OS
			}
			FreeLibrary(hShcore);
		}

		if (result != aware::unaware)
			return result;

		HMODULE hUser32 = LoadLibrary(_T("user32.dll"));
		typedef BOOL(*SetProcessDPIAwareFunc)();
		SetProcessDPIAwareFunc setDPIAware = (SetProcessDPIAwareFunc)GetProcAddress(hUser32, "SetProcessDPIAware");
		if (setDPIAware) {
			if (setDPIAware())
				result = aware::system;
		}
		FreeLibrary(hUser32);

		return result;
	}
};

CAppModule _Module;

int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	CTasksFrame wndMain;
	int nRet = 0;

	Strings _;

	pm::PostMortemSupport([&] {
		_.init();
		_.path_manager<locale::manager::ExtensionPath>((fs::app_directory() / "locale").string(), "Tasks");
		_.open("en"); // TODO: detect the language form the OS

		wndMain._ = _;

#if BULID_NUMBER_IN_TITLE
		auto title = utf::widen(
			_(lng::LNG_APP_TITLE,
				_(lng::LNG_APP_NAME),
				PROGRAM_VERSION_STRING PROGRAM_VERSION_STABILITY,
				PROGRAM_VERSION_BUILD
				)
			);
#else
		auto title = utf::widen(tr(lng::LNG_APP_NAME));
#endif

		if (wndMain.Create(NULL, NULL, title.c_str()) == NULL)
		{
			ATLTRACE(_T("Main window creation failed!\n"));
			return;
		}

		wndMain.rebuildAccel();
		if (!wndMain.updatePosition()) {
			WINDOWPLACEMENT placement = { sizeof(WINDOWPLACEMENT) };
			placement.showCmd = nCmdShow;

			RECT workArea;
			if (SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0)) {
				workArea.right -= workArea.left;
				workArea.bottom -= workArea.top;
				workArea.left = workArea.top = 0;

				auto padding = GetSystemMetrics(SM_CYCAPTION);
				workArea.left += padding;
				workArea.top += padding;
				workArea.right -= padding;
				workArea.bottom -= padding;

				workArea.left = (workArea.right + workArea.left) / 2;
				workArea.top = (workArea.bottom + workArea.top) / 2;
				placement.rcNormalPosition = workArea;
				wndMain.SetWindowPlacement(&placement);
			}
			else
				wndMain.ShowWindow(nCmdShow);
		}

		nRet = theLoop.Run();

		_Module.RemoveMessageLoop();
	});

	return nRet;
}

class Fonts {
	std::vector<fs::path> m_fonts;
public:
	Fonts()
	{
		for (auto entry : fs::dir(exe_dir() / "fonts"))
		{
			if (entry.status().is_directory())
				continue;
			auto count = AddFontResourceEx(entry.path().wnative().c_str(), FR_PRIVATE, nullptr);
			if (count)
				m_fonts.push_back(entry.path());
		}
	}

	~Fonts()
	{
		for (auto& path : m_fonts)
			RemoveFontResourceExW(path.wnative().c_str(), FR_PRIVATE, nullptr);
	}
};

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
	Fonts external;

	HRESULT hRes = ::CoInitialize(NULL);
	// If you are running on NT 4.0 or higher you can use the following call instead to 
	// make the EXE free threaded. This means that calls come in on a random RPC thread.
	//	HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	ATLASSERT(SUCCEEDED(hRes));

	dpi::checkHighDpi();

	ULONG_PTR gdiplusToken;
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	net::http::client::set_program_client_info(PROGRAM_NAME "/" PROGRAM_VERSION_STRING);

	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(NULL, 0, 0, 0L);

	AtlInitCommonControls(ICC_COOL_CLASSES | ICC_BAR_CLASSES);	// add flags to support other controls

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet = Run(lpstrCmdLine, nCmdShow);

	_Module.Term();

	Gdiplus::GdiplusShutdown(gdiplusToken);
	::CoUninitialize();

	return nRet;
}
