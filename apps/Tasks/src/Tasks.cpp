// [!output PROJECT_NAME].cpp : main source file for [!output PROJECT_NAME].exe
//

#include "stdafx.h"

#include "resource.h"

#include "TasksView.h"
#include "AboutDlg.h"
#include "TasksFrame.h"

#include <shellapi.h>

#include <memory>

CAppModule _Module;

int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
	//int argc = 0;
	//LPWSTR * argv = nullptr;
	//std::unique_ptr<LPWSTR, decltype(&LocalFree)> args{ nullptr, LocalFree };
	//args.reset(CommandLineToArgvW(GetCommandLineW(), &argc));
	//if (args) argv = args.get();
	//else argc = 0;

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

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
	HRESULT hRes = ::CoInitialize(NULL);
// If you are running on NT 4.0 or higher you can use the following call instead to 
// make the EXE free threaded. This means that calls come in on a random RPC thread.
//	HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	ATLASSERT(SUCCEEDED(hRes));

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
