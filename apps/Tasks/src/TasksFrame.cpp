// TasksFrame.cpp : implmentation of the CTasksFrame class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "version.h"

#include "AboutDlg.h"
#include "ConnectionDlg.h"
#include "TasksView.h"
#include "TasksFrame.h"

#include <jira/jira.hpp>
#include <jira/server.hpp>
#include <net/utf8.hpp>
#include <net/xhr.hpp>
#include <sstream>

#include "AppSettings.h"

#include "wincrypt.h"
#pragma comment(lib, "crypt32.lib")

#undef max

#include <algorithm>

#define WIDE2(x) L ## x
#define WIDE(x) WIDE2(x)

#ifndef BULID_NUMBER_IN_TITLE
#define BULID_NUMBER_IN_TITLE 1
#endif

#ifdef TRAY_USE_GUID
// {FC7805FE-9587-4985-A8D2-4260E1FCD61A}
static const GUID UUID_TrayIcon{ 0xfc7805fe, 0x9587, 0x4985, { 0xa8, 0xd2, 0x42, 0x60, 0xe1, 0xfc, 0xd6, 0x1a } };

// {599A2310-CA40-43F8-B885-D0376BC954CB}
static const GUID UUID_AttentionIcon{ 0x599a2310, 0xca40, 0x43f8, { 0xb8, 0x85, 0xd0, 0x37, 0x6b, 0xc9, 0x54, 0xcb } };
#endif

constexpr UINT TRAYICON_MAIN = 1000;
constexpr UINT TRAYICON_ATTENTION = 1001;

std::wstring CTasksFrame::buildTitle()
{
#if BULID_NUMBER_IN_TITLE
	auto title = utf::widen(
		_.tr(lng::LNG_APP_TITLE,
			_.tr(lng::LNG_APP_NAME),
			PROGRAM_VERSION_STRING PROGRAM_VERSION_STABILITY,
			PROGRAM_VERSION_BUILD
			)
		);
#else
	auto title = utf::widen(_(lng::LNG_APP_NAME));
#endif
	if (m_elevated)
		title = utf::widen(_.tr(lng::LNG_APP_TITLE_ELEVATED, utf::narrowed(title)));

	return title;
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

LRESULT CTasksFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
	// Check if Common Controls 6.0 are used. If yes, use 32-bit (alpha) images
	// for the toolbar and command bar. If not, use the old, 4-bit images.
	UINT uResID = IDR_MAINFRAME_OLD;
	DWORD dwMajor = 0;
	DWORD dwMinor = 0;
	HRESULT hRet = AtlGetCommCtrlVersion(&dwMajor, &dwMinor);
	if (SUCCEEDED(hRet) && dwMajor >= 6)
		uResID = IDR_MAINFRAME;

	createIcons();
	createItems(_.tr);
	SetMenu(createAppMenu(_.tr));
	createAppToolbar(_.tr, [this](const std::initializer_list<menu::item>& items) {
		m_hWndToolBar = createToolbar(items, m_hWnd);
	});

	_.onupdate([this] {
		createItems(_.tr);
		SetMenu(createAppMenu(_.tr));
		SetWindowTextW(buildTitle().c_str());
	});

	m_model->setTimerHandle(m_hWnd);

	m_view.m_elevated = m_elevated;
	m_view.m_model = m_model;
	m_view._.tr = _.tr;

	m_hWndClient = m_container.Create(m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0);

	m_view.setScroller(this);

	m_view.setNotifier([&](const std::wstring& title, const std::wstring& message) {
		if (!m_attentionIcon.IsInstalled()) {
			auto tray_icon = (HICON)LoadImage(_Module.GetResourceInstance(),
				MAKEINTRESOURCE(IDR_ATTENTION), IMAGE_ICON,
				GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
				LR_DEFAULTCOLOR);
			auto ICON_TITLE = _.tr(lng::LNG_ATTENTION_TOOLTIP);
#ifdef TRAY_USE_GUID
			m_attentionIcon.Install(m_hWnd, UUID_AttentionIcon, tray_icon, nullptr, utf::widen(ICON_TITLE).c_str());
#else
			m_attentionIcon.Install(m_hWnd, TRAYICON_ATTENTION, tray_icon, nullptr, utf::widen(ICON_TITLE).c_str());
#endif
		}
		m_balloonVisible = true;
		m_attentionIcon.ShowBalloon(title.c_str(), message.c_str());
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

	auto toolbar_icon = (HICON)LoadImage(_Module.GetResourceInstance(),
		MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON,
		GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
		LR_DEFAULTCOLOR);
	auto createStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
#ifdef TRAY_USE_GUID
	m_taskIcon.Install(m_hWnd, UUID_TrayIcon, toolbar_icon, toolbar_menu, createStruct->lpszName);
#else
	m_taskIcon.Install(m_hWnd, TRAYICON_MAIN, toolbar_icon, toolbar_menu, createStruct->lpszName);
#endif

	m_credUI->setHandle(m_hWnd);
	m_model->startup(m_credUI);

#if 0 // startup_info usage example:
	if (startup_info.type != StartupType::Normal) {
		std::wstring msg = startup_info.type == StartupType::Fresh ?
			L"Wow, hello, how are you?" :
			L"Hope this version is as nice as " + utf::widen(startup_info.previous);
		MessageBoxW(msg.c_str());
	}
#endif

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

LRESULT CTasksFrame::OnAttentionIconClick(LPARAM /*uMsg*/, BOOL& /*bHandled*/)
{
	if (!IsWindowVisible())
		ShowWindow(SW_SHOW);
	SetForegroundWindow(m_hWnd);
	PostMessage(WM_NULL, 0, 0);

	m_attentionIcon.Uninstall();
	m_balloonVisible = false;

	return 0;
}

LRESULT CTasksFrame::OnAttentionIconTimeoutOrHide(LPARAM /*uMsg*/, BOOL& /*bHandled*/)
{
	m_balloonVisible = false;
	auto active = GetActiveWindow();
	if ((m_hWnd == active || IsChild(active)) && IsWindowVisible())
		m_attentionIcon.Uninstall();
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

LRESULT CTasksFrame::OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
	bHandled = FALSE;

	synchronize(*m_model, [&] {
		auto local = m_model->servers();
		for (auto info : local) {
			if (info.m_server->sessionId() == wParam) {
				KillTimer(wParam);
				bHandled = TRUE;

				std::thread{ [info] {
					info.m_server->loadFields(info.m_document);
					info.m_server->refresh(info.m_document);
				} }.detach();
				break;
			}
		}
	});

#ifdef LCLIK_TASK_MENU
	if (!bHandled && wParam == uEvent) {
		bHandled = TRUE;
		KillTimer(uEvent);
		m_taskIcon.OnTaskbarContextMenu(WM_RBUTTONDOWN, bHandled);
	}
#endif

	return 0;
}

LRESULT CTasksFrame::OnActivate(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
	bHandled = FALSE;
	if (wParam != WA_INACTIVE && IsWindowVisible() && !m_balloonVisible)
		m_attentionIcon.Uninstall();

	return 0;
}

LRESULT CTasksFrame::OnUninstall(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	tasks_exit->call();
	return 0;
}

typedef HRESULT(_stdcall *RegisterApplicationRestartT)(PCWSTR, DWORD);
static HRESULT RegisterApplicationRestartStub(PCWSTR pwzCommandline, DWORD dwFlags)
{
	HMODULE hModule = ::LoadLibrary(_T("kernel32.dll"));
	if (hModule == NULL)
		return E_FAIL;
	RegisterApplicationRestartT proc =
		reinterpret_cast<RegisterApplicationRestartT>(::GetProcAddress(hModule, "RegisterApplicationRestart"));
	if (proc == NULL) {
		::FreeLibrary(hModule);
		return E_FAIL;
	}
	HRESULT ret = (proc)(pwzCommandline, dwFlags);
	::FreeLibrary(hModule);
	return ret;
}

LRESULT CTasksFrame::OnQueryEndSession(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	RegisterApplicationRestartStub(L"--quiet", 0);
	PostMessage(WM_COMMAND, tasks_exit->id(), 0);
	return TRUE;
}

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
	else {
		ShowWindow(SW_SHOW);
		SetForegroundWindow(m_hWnd);
		PostMessage(WM_NULL, 0, 0);
	}
}

void CTasksFrame::newConnection()
{
	CConnectionDlg dlg;
	if (dlg.DoModal(m_hWnd) == IDOK) {
		auto conn = std::make_shared<jira::server>(dlg.serverName, dlg.userName, dlg.userPassword, dlg.serverUrl, std::vector<jira::search_def>{});
		m_model->add(conn);
	};
}

void CTasksFrame::refreshAll()
{
	synchronize(*m_model, [&] {
		auto local = m_model->servers();
		for (auto info : local) {
			std::thread{ [info] {
				info.m_server->loadFields(info.m_document);
				info.m_server->refresh(info.m_document);
			} }.detach();
		}
	});
}

void CTasksFrame::setLanguage(const std::string& lang)
{
	CAppSettings { }.language(lang);

	if (lang.empty() || !_.tr.open(lang))
		_.tr.open_first_of(locale::system_locales());
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
	CAboutDlg dlg {_.tr};
	dlg.DoModal();
}

void CTasksFrame::setContentSize(size_t width, size_t height)
{
	m_container.SetScrollSize(width, height, TRUE, false);
	RECT client;
	m_container.GetClientRect(&client);
	m_view.SetWindowPos(nullptr, 0, 0,
		std::max(width, (size_t)client.right),
		std::max(height, (size_t)client.bottom),
		SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
}

void CTasksFrame::scrollIntoView(long left, long top, long right, long bottom)
{
	RECT r {left, top, right, bottom};
	m_container.ScrollToView(r);
}
