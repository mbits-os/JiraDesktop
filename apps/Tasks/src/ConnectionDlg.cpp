// aboutdlg.cpp : implementation of the CAboutDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "ConnectionDlg.h"
#include <net/utf8.hpp>
#include <net/uri.hpp>
#include <jira/server.hpp>
#include <atlstr.h>

void CConnectionDlg::setWindowText(const std::string& value, int id)
{
	SetDlgItemText(id, utf::widen(value).c_str());
}

std::string CConnectionDlg::getWindowText(int id)
{
	ATL::CString text;
	GetDlgItemText(id, text);
	return utf::narrowed({ (LPCWSTR)text, (size_t)text.GetLength() });
}

bool CConnectionDlg::hasText(int id)
{
	return GetDlgItem(id).GetWindowTextLength() > 0;
}

namespace {
	bool isURL_(const Uri& uri) {
		if (uri.relative() || uri.opaque())
			return false;

		auto auth = uri.authority();
		if (auth.empty())
			return false;

		auto at = auth.find('@');
		if (at != std::string::npos)
			auth = auth.substr(at + 1);

		return !auth.empty(); // potentialy: check if authority is a valid <server> or <server>:<port> pair...
	}

	bool isURL(const std::string& url) {
		Uri uri{ url };
		bool ret = isURL_(uri);
		if (!ret && uri.relative()) {
			uri = "http://" + url;
			ret = isURL_(uri);
		}

		return ret;
	}
}

void CConnectionDlg::updateExitState()
{
	GetDlgItem(IDOK).EnableWindow(hasText(IDC_URL) && (m_urlTestStage == URL_VALID));
}

LRESULT CConnectionDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CenterWindow(GetParent());

	m_credUI->setHandle(m_hWnd, _, m_model);

	setWindowText(serverName, IDC_NAME);
	setWindowText(serverUrl, IDC_URL);

	{
		LOGFONT lf = { 0 };
		CFontHandle{ GetFont() }.GetLogFont(lf);
		lf.lfHeight *= 38;
		lf.lfHeight /= 8;
		m_symbols.CreateFont(lf.lfHeight, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
			L"FontAwesome");
	}

	SendDlgItemMessage(IDC_STATIC_SERVER, (WPARAM)(HFONT)m_symbols, TRUE);
	SendDlgItemMessage(IDC_STATIC_USER,   (WPARAM)(HFONT)m_symbols, TRUE);
	SetDlgItemText(IDC_STATIC_SERVER, L"\xf233");
	SetDlgItemText(IDC_STATIC_USER, L"\xf007");

	updateExitState();

	return TRUE;
}

// defined in AppModel.cpp
std::shared_ptr<gui::document> make_document(const std::shared_ptr<jira::server>& srvr, const gui::credential_ui_ptr& ui);

void CConnectionDlg::testURL(std::string url)
{
	if (isURL(url)) {
		Uri uri { url };
		if (uri.relative()) {
			url = "http://" + url;
		}

		GetDlgItem(IDC_NAME).EnableWindow(FALSE);
		auto handle = m_hWnd;
		auto counter = ++m_urlTestCounter;
		m_urlTestStage = ACTIVE;
		jira::server::find_root(make_document({ }, m_credUI), url, [handle, counter](const jira::server_info& info) {
			::SendMessage(handle, UM_SERVER_INFO, counter, (LPARAM)&info);
		});
	}
}

LRESULT CConnectionDlg::OnServerInfo(UINT, WPARAM wParam, LPARAM lParam, BOOL &)
{
	if (wParam != m_urlTestCounter)
		return 0;

	bool valid = false;
	auto info = reinterpret_cast<const jira::server_info*>(lParam);
	if (info && !info->baseUrl.empty()) {
		valid = true;
		if (!info->serverTitle.empty())
			setWindowText(info->serverTitle, IDC_NAME);
		if (info->baseUrl != getWindowText(IDC_URL)) {
			m_updatingUrl = true;
			setWindowText(info->baseUrl, IDC_URL);
			m_updatingUrl = false;
		}
	}
	GetDlgItem(IDC_NAME).EnableWindow();
	m_urlTestStage = valid ? URL_VALID : URL_INVALID;
	MessageBeep(valid ? MB_ICONINFORMATION : MB_ICONERROR);

	updateExitState();

	return 0;
}

LRESULT CConnectionDlg::OnTextChanged(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	updateExitState();
	if (wID == IDC_URL && hasText(IDC_URL) && !m_updatingUrl)
		testURL(getWindowText(IDC_URL));

	return 0;
}

LRESULT CConnectionDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if (wID == IDOK) {
		serverName = getWindowText(IDC_NAME);
		serverUrl = getWindowText(IDC_URL);
	}

	EndDialog(wID);
	return 0;
}
