// TasksFrame.cpp : implmentation of the CTasksFrame class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "aboutdlg.h"
#include "TasksView.h"
#include "TasksFrame.h"

#include <jira/jira.hpp>
#include <jira/server.hpp>
#include <net/utf8.hpp>
#include <net/xhr.hpp>
#include <sstream>
#include <thread>

#include "AppSettings.h"

#include "wincrypt.h"
#pragma comment(lib, "crypt32.lib")


std::string contents(LPCWSTR path)
{
	std::unique_ptr<FILE, decltype(&fclose)> f{ _wfopen(path, L"r"), fclose };
	if (!f)
		return std::string();

	std::string out;
	char buffer[8192];
	int read = 0;
	while ((read = fread(buffer, 1, sizeof(buffer), f.get())) > 0)
		out.append(buffer, buffer + read);
	return out;
}

void print(FILE* f, const std::string& s)
{
	fwrite(s.c_str(), 1, s.length(), f);
}

void print(FILE* f, const char* s)
{
	if (!s)
		return;
	fwrite(s, 1, strlen(s), f);
}

template <size_t length>
void print(FILE* f, const char(&s)[length])
{
	if (!s)
		return;
	fwrite(s, 1, strlen(s), f);
}

BOOL CTasksFrame::PreTranslateMessage(MSG* pMsg)
{
	if (CFameSuper::PreTranslateMessage(pMsg))
		return TRUE;

	return m_view.PreTranslateMessage(pMsg);
}

BOOL CTasksFrame::OnIdle()
{
	UIUpdateToolBar();
	return FALSE;
}

LRESULT CTasksFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// Check if Common Controls 6.0 are used. If yes, use 32-bit (alpha) images
	// for the toolbar and command bar. If not, use the old, 4-bit images.
	UINT uResID = IDR_MAINFRAME_OLD;
	DWORD dwMajor = 0;
	DWORD dwMinor = 0;
	HRESULT hRet = AtlGetCommCtrlVersion(&dwMajor, &dwMinor);
	if (SUCCEEDED(hRet) && dwMajor >= 6)
		uResID = IDR_MAINFRAME;

	CreateSimpleToolBar(uResID);

	m_view.m_model = m_model;
	m_hWndClient = m_view.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0);
	m_font = AtlCreateControlFont();
	m_view.SetFont(m_font);

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	m_taskIcon.Install(m_hWnd, 1, IDR_TASKBAR);

	auto hwnd = m_hWnd;

	m_model->startup();

	return 0;
}

LRESULT CTasksFrame::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	// unregister message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);

	bHandled = FALSE;
	return 1;
}

LRESULT CTasksFrame::OnTaskIconClick(LPARAM /*uMsg*/, BOOL& /*bHandled*/)
{
	return 0;
}

LRESULT CTasksFrame::OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	PostMessage(WM_CLOSE);
	return 0;
}

LRESULT CTasksFrame::OnFileNew(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: add code to initialize document

	return 0;
}

LRESULT CTasksFrame::OnTasksRefersh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	auto local = m_model->servers();
	for (auto server : local) {
		std::thread{ [server] {
			server->loadFields();
			server->refresh();
		} }.detach();
	}

	return 0;
}

LRESULT CTasksFrame::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CAboutDlg dlg;
	dlg.DoModal();
	return 0;
}
