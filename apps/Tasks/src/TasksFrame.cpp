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

bool CTasksFrame::updatePosition()
{
	CAppSettings settings;
	if (settings.getType("Placement") == settings::Binary) {
		auto bytes = settings.getBinary("Placement");
		//WINDOWPLACEMENT placement = { sizeof(WINDOWPLACEMENT) };
		if (bytes.size() == sizeof(WINDOWPLACEMENT)) {
			auto pos = (const WINDOWPLACEMENT*)bytes.data();
			if (pos->length == sizeof(WINDOWPLACEMENT)) {
				SetWindowPlacement(pos);
				return true;
			}
		}
	}

	return false;
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

#if 1
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

	auto toolbar_menu = createMenuBar({ menu::popup({
		toolbar_default,
		menu::separator(),
		tasks_new,
		tasks_refresh,
		menu::separator(),
		help_licences,
		help_about,
		menu::separator(),
		tasks_exit
	})});

	auto toolbar_icon = icon_taskbar->getNativeIcon(GetSystemMetrics(SM_CXSMICON));
	icon_taskbar->detachIcon();

	m_taskIcon.Install(m_hWnd, 1, toolbar_icon, toolbar_menu);

	icon_frame->getNativeIcon(GetSystemMetrics(SM_CXSMICON));
	m_smallIcon = icon_frame->detachIcon();

	icon_frame->getNativeIcon(GetSystemMetrics(SM_CXICON));
	m_bigIcon = icon_frame->detachIcon();

	SetIcon(m_smallIcon, FALSE);
	SetIcon(m_bigIcon, TRUE);

	auto hwnd = m_hWnd;

	m_model->startup();

	return 0;
}

LRESULT CTasksFrame::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	WINDOWPLACEMENT placement = { sizeof(WINDOWPLACEMENT) };
	if (GetWindowPlacement(&placement)) {
		CAppSettings settings;
		settings.setBinary("Placement", placement);
	}

	// unregister message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);

	bHandled = FALSE;
	return 1;
}

#ifdef LCLIK_TASK_MENU
static UINT_PTR uEvent = 0x0110FFEF;

LRESULT CTasksFrame::OnTaskIconClick(LPARAM /*uMsg*/, BOOL& /*bHandled*/)
{
	SetTimer(uEvent, GetDoubleClickTime());
	return 0;
}
#endif

LRESULT CTasksFrame::OnTaskIconDefault(LPARAM /*uMsg*/, BOOL& /*bHandled*/)
{
#ifdef LCLIK_TASK_MENU
	KillTimer(uEvent);
#endif
	toolbar_default->call();
	return 0;
}


LRESULT CTasksFrame::OnCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
	bHandled = onCommand(LOWORD(wParam)) ? TRUE : FALSE;
	return 0;
}

LRESULT CTasksFrame::OnSysCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
	if (wParam == SC_MINIMIZE)
		ShowWindow(SW_HIDE);
	else
		bHandled = FALSE;
	return 0;
}

#ifdef LCLIK_TASK_MENU
LRESULT CTasksFrame::OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
	if (wParam == uEvent) {
		KillTimer(uEvent);
		m_taskIcon.OnTaskbarContextMenu(WM_RBUTTONDOWN, bHandled);
	} else
		bHandled = FALSE;

	return 0;
}
#endif

LRESULT CTasksFrame::OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	ShowWindow(SW_HIDE);
	return 0;
}

std::string Wide2ACP(const std::wstring& s) {
	auto size = WideCharToMultiByte(CP_ACP, 0, (wchar_t*)s.c_str(), -1, nullptr, 0, nullptr, nullptr);
	std::unique_ptr<char[]> out{ new char[size + 1] };
	WideCharToMultiByte(CP_ACP, 0, (wchar_t*)s.c_str(), -1, out.get(), size + 1, nullptr, nullptr);
	return out.get();
}

LRESULT CTasksFrame::OnToolTipTextA(int idCtrl, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
	LPNMTTDISPINFOA pDispInfo = (LPNMTTDISPINFOA)pnmh;
	if ((idCtrl != 0) && !(pDispInfo->uFlags & TTF_IDISHWND))
	{
		const int cchBuff = 256;
		char szBuff[cchBuff] = { 0 };
		auto action = m_menubarManager.find(LOWORD(idCtrl));
		if (action) {
			auto tool = action->tooltip();
			if (!tool.empty()) {
				auto key = action->hotkey().name();
				if (!key.empty())
					tool += " (" + key + ")";

				auto ui = Wide2ACP(utf::widen(tool));
				SecureHelper::strncpyA_x(pDispInfo->szText, _countof(pDispInfo->szText), ui.c_str(), _TRUNCATE);
#if (_WIN32_IE >= 0x0300)
				pDispInfo->uFlags |= TTF_DI_SETITEM;
#endif // (_WIN32_IE >= 0x0300)
			}
		}
	}

	return 0;
}

LRESULT CTasksFrame::OnToolTipTextW(int idCtrl, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
	LPNMTTDISPINFOW pDispInfo = (LPNMTTDISPINFOW)pnmh;
	if ((idCtrl != 0) && !(pDispInfo->uFlags & TTF_IDISHWND))
	{
		const int cchBuff = 256;
		char szBuff[cchBuff] = { 0 };
		auto action = m_menubarManager.find(LOWORD(idCtrl));
		if (action) {
			auto tool = action->tooltip();
			if (!tool.empty()) {
				auto key = action->hotkey().name();
				if (!key.empty())
					tool += " (" + key + ")";

				auto ui = utf::widen(tool);
				SecureHelper::strncpyW_x(pDispInfo->szText, _countof(pDispInfo->szText), ui.c_str(), _TRUNCATE);
#if (_WIN32_IE >= 0x0300)
				pDispInfo->uFlags |= TTF_DI_SETITEM;
#endif // (_WIN32_IE >= 0x0300)
			}
		}
	}

	return 0;
}

void CTasksFrame::showHide()
{
	if (IsWindowVisible())
		ShowWindow(SW_HIDE);
	else
		ShowWindow(SW_SHOW);
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
	synchronize(*m_model, [&] {
		auto local = m_model->servers();
		for (auto info : local) {
			std::thread{ [info] {
				info.m_server->loadFields();
				info.m_server->refresh(info.m_document);
			} }.detach();
		}
	});
}

void CTasksFrame::exitApplication()
{
	DestroyWindow();
}

void CTasksFrame::showLicence()
{
}

void CTasksFrame::about()
{
	CAboutDlg dlg;
	dlg.DoModal();
}
