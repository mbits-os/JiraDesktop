// [!output PROJECT_NAME].cpp : main source file for [!output PROJECT_NAME].exe
//

#include "stdafx.h"

#include "resource.h"
#include "version.h"

#include "TasksView.h"
#include "AboutDlg.h"
#include "TasksFrame.h"

#include <shellapi.h>

#include <memory>
#include <net/filesystem.hpp>
#include <net/xhr.hpp>

namespace fs = filesystem;

fs::path exe_dir() {
	static fs::path dir = fs::app_directory();
	return dir;
}

CAppModule _Module;

int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	CTasksFrame wndMain;

	if(wndMain.CreateEx() == NULL)
	{
		ATLTRACE(_T("Main window creation failed!\n"));
		return 0;
	}

	wndMain.ShowWindow(nCmdShow);

	int nRet = theLoop.Run();

	_Module.RemoveMessageLoop();
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
	HRESULT hRes = ::CoInitialize(NULL);
// If you are running on NT 4.0 or higher you can use the following call instead to 
// make the EXE free threaded. This means that calls come in on a random RPC thread.
//	HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	ATLASSERT(SUCCEEDED(hRes));

	Fonts external;

	net::http::client::set_program_client_info(PROGRAM_NAME "/" PROGRAM_VERSION_STRING);

	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(NULL, 0, 0, 0L);

	AtlInitCommonControls(ICC_COOL_CLASSES | ICC_BAR_CLASSES);	// add flags to support other controls

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet = Run(lpstrCmdLine, nCmdShow);

	_Module.Term();

	::CoUninitialize();

	return nRet;
}
