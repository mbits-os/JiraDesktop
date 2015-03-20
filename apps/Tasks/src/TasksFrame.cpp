// TasksFrame.cpp : implmentation of the CTasksFrame class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "aboutdlg.h"
#include "TasksView.h"
#include "TasksFrame.h"

#include <jira/jira.hpp>
#include <sstream>

std::basic_string<wchar_t> utf8_to_utf16(const std::string& s)
{
	auto size = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
	std::unique_ptr<wchar_t []> out{ new wchar_t[size + 1] };
	MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, (wchar_t*) out.get(), size + 1);
	return out.get();
}

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

void CTasksFrame::load(LPCWSTR path)
{
	json::map data{ json::from_string(contents(path)) };
	std::ostringstream o;
	auto startAt = data["startAt"].as_int();
	auto total = data["total"].as_int();
	json::vector issues{ data["issues"] };

	const char* columns [] = {
		"status",
		"assignee",
		"key",
		"priority",
		"summary"
	};

	jira::db db{ "https://cam.sprc.samsung.pl" };
	auto model = db.create_model(columns);

	std::vector<jira::record> dataset;

	for (auto& v_issue : issues) {
		json::map issue{ v_issue };
		json::map fields{ issue["fields"] };
		auto key = issue["key"].as_string();
		auto id = issue["id"].as_string();

		dataset.push_back(model.visit(fields, key, id));
	}

	o << "Issues " << (startAt + 1) << '-' << (startAt + issues.size()) << " of " << total << ":\n";
	OutputDebugString(utf8_to_utf16(o.str()).c_str()); o.str("");
	for (auto&& row : dataset)
		OutputDebugString(utf8_to_utf16(row.text(" | ") + "\n").c_str());

	std::unique_ptr<FILE, decltype(&fclose)> f{ fopen("issues.html", "w"), fclose };
	if (!f)
		return;

	print(f.get(), R"(<style>
body, td {
	font-family: Arial, sans-serif;
	font-size: 12px
}
a {
	color: #3b73af;
	text-decoration: none;
}

a:hover {
	text-decoration: underline;
}
</style>
)");
	o << "<p>Issues " << (startAt + 1) << '-' << (startAt + issues.size()) << " of " << total << ":</p>\n<table>\n";
	print(f.get(), o.str()); o.str("");
	for (auto&& row : dataset)
		print(f.get(), "  <tr>\n    <td>" + row.html("</td>\n    <td>") + "</td>\n  </tr>\n");
	print(f.get(), "</table>\n");
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

	// Set values to be displayed in the view window
	// m_view.m_dwCommCtrlMajor = dwMajor;
	// m_view.m_dwCommCtrlMinor = dwMinor;
	// m_view.m_bAlpha = (uResID == IDR_MAINFRAME);

	// create command bar window
	HWND hWndCmdBar = m_CmdBar.Create(m_hWnd, rcDefault, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE);
	// attach menu
	m_CmdBar.AttachMenu(GetMenu());
	// load command bar images
	m_CmdBar.LoadImages(uResID);
	// remove old menu
	SetMenu(NULL);

	HWND hWndToolBar = CreateSimpleToolBarCtrl(m_hWnd, uResID, FALSE, ATL_SIMPLE_TOOLBAR_PANE_STYLE);

	CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
	AddSimpleReBarBand(hWndCmdBar);
	AddSimpleReBarBand(hWndToolBar, NULL, TRUE);

	m_hWndClient = m_view.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0);
	m_font = AtlCreateControlFont();
	m_view.SetFont(m_font);

	UIAddToolBar(hWndToolBar);
	UISetCheck(ID_VIEW_TOOLBAR, 1);
	UISetCheck(ID_VIEW_STATUS_BAR, 1);

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	m_taskIcon.Install(m_hWnd, 1, IDR_MAINFRAME);

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

LRESULT CTasksFrame::OnFilePrint(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	WTL::CPrintDialogEx dlg;
	dlg.DoModal();
	return 0;
}

LRESULT CTasksFrame::OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	static BOOL bVisible = TRUE;	// initially visible
	bVisible = !bVisible;
	CReBarCtrl rebar = m_hWndToolBar;
	int nBandIndex = rebar.IdToIndex(ATL_IDW_BAND_FIRST + 1);	// toolbar is 2nd added band
	rebar.ShowBand(nBandIndex, bVisible);
	UISetCheck(ID_VIEW_TOOLBAR, bVisible);
	UpdateLayout();
	return 0;
}

LRESULT CTasksFrame::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CAboutDlg dlg;
	dlg.DoModal();
	return 0;
}
