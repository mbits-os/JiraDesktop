// TasksFrame.cpp : implmentation of the CTasksFrame class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "AboutDlg.h"
#include "ConnectionDlg.h"
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

#undef max

#include <algorithm>

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

void CTasksFrame::rebuildAccel()
{
	m_hAccel = m_menubarManager.createAccel();
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

	createItems();

	//CreateSimpleToolBar(uResID);

	m_view.m_model = m_model;

	m_hWndClient = m_container.Create(m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0);

	m_view.setScroller([&](size_t width, size_t height) {
		m_container.SetScrollSize(width, height, TRUE, false);
		RECT client;
		m_container.GetClientRect(&client);
		m_view.SetWindowPos(nullptr, 0, 0,
			std::max(width, (size_t)client.right),
			std::max(height, (size_t)client.bottom),
			SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
	});
	m_view.Create(m_container, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0);
	m_container.SetClient(m_view, false);

#if 0
	m_font = AtlCreateControlFont();
#else
	int fontSize = 14;
	{
		CWindowDC dc{ m_hWnd };
		fontSize = dc.GetDeviceCaps(LOGPIXELSY) * fontSize / 96;
	}
	m_font.CreateFont(-fontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
		DEFAULT_PITCH | FF_SWISS, L"Arial");
#endif
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

LRESULT CTasksFrame::OnCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
	bHandled = onCommand(LOWORD(wParam)) ? TRUE : FALSE;
	return 0;
}

void CTasksFrame::newConnection()
{
	CConnectionDlg dlg;
	if (dlg.DoModal() == IDOK) {
		auto conn = std::make_shared<jira::server>(dlg.serverName, dlg.userName, dlg.userPassword, dlg.serverUrl, jira::search_def{});
		m_model->add(conn);
	};
}

void CTasksFrame::refreshAll()
{
	auto local = m_model->servers();
	auto document = m_model->document();
	for (auto server : local) {
		std::thread{ [server, document] {
			server->loadFields();
			server->refresh(document);
		} }.detach();
	}
}

void CTasksFrame::exitApplication()
{
	PostMessage(WM_CLOSE);
}

void CTasksFrame::showLicence()
{
}

void CTasksFrame::about()
{
	CAboutDlg dlg;
	dlg.DoModal();
}
