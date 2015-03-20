// TasksFrame.cpp : implmentation of the CTasksFrame class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "aboutdlg.h"
#include "TasksView.h"
#include "TasksFrame.h"

#include <jira.h>

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

class jiraDb {
public:

	template <typename T>
	inline void field(const char* name)
	{

	}

	void field_type(const char* name, const char* type, const char* display)
	{
	}

	jiraDb()
	{
		field<jira::key>("key");
		field<jira::string>("string");
		field<jira::user>("user");
		field<jira::icon>("icon");

		field_type("key", "key", "Key");
		field_type("summary", "string", "Summary");
		field_type("description", "string", "Description");
		field_type("issuetype", "icon", "Issue Type");
		field_type("priority", "icon", "Priority");
		field_type("status", "icon", "Status");
		field_type("reporter", "user", "Reporter");
		field_type("creator", "user", "Creator");
		field_type("assignee", "user", "Assignee");
	}

	const jira::type* get(const char* column) const
	{
		return nullptr;
	}

	template <size_t length>
	std::string visit(const json::map& object, const std::string& key, const char* (&columns)[length]) const
	{
		std::ostringstream o;
		bool first = true;
		for (auto& col : columns) {
			if (first) first = false;
			else o << " | ";
			auto col_type = get(col);
			if (col_type)
				o << col_type->visit(object, key);
			else
				o << col << ":(nullptr)";
		}
		return o.str();
	}
};
void CTasksFrame::load(LPCWSTR path)
{
	json::map data{ json::from_string(contents(path)) };
	std::ostringstream o;
	auto startAt = data["startAt"].as_int();
	auto total = data["total"].as_int();
	json::vector issues{ data["issues"] };
	o << "Issues " << (startAt + 1) << '-' << (startAt + issues.size()) << " of " << total << ":\n";
	OutputDebugStringA(o.str().c_str()); o.str("");

	jiraDb db;

	const char* columns[] = {
		"assignee",
		"issuetype",
		"key",
		"status",
		"summary"
	};

	jira::row row{};
	row
		.column<jira::user>("assignee", nullptr, ",")
		.column<jira::icon>("issuetype")
		.column<jira::key>("key", "[", "]")
		.column<jira::icon>("status")
		.column<jira::string>("summary", "\"", "\"")
		;

	for (auto& v_issue : issues) {
		json::map issue{ v_issue };
		json::map fields{ issue["fields"] };
		auto key = issue["key"].as_string();
		OutputDebugStringA((db.visit(fields, key, columns) + "\n").c_str());
	}
}

BOOL CTasksFrame::PreTranslateMessage(MSG* pMsg)
{
	if(CFrameWindowImpl<CTasksFrame>::PreTranslateMessage(pMsg))
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

	CreateSimpleStatusBar();

	m_hWndClient = m_view.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_CLIENTEDGE);
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

LRESULT CTasksFrame::OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	BOOL bVisible = !::IsWindowVisible(m_hWndStatusBar);
	::ShowWindow(m_hWndStatusBar, bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
	UISetCheck(ID_VIEW_STATUS_BAR, bVisible);
	UpdateLayout();
	return 0;
}

LRESULT CTasksFrame::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CAboutDlg dlg;
	dlg.DoModal();
	return 0;
}
