// aboutdlg.cpp : implementation of the CAboutDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "ConnectionDlg.h"
#include <net/utf8.hpp>
#include <net/uri.hpp>
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
	GetDlgItem(IDOK).EnableWindow(hasText(IDC_URL) && hasText(IDC_LOGIN) && hasText(IDC_PASSWORD) && isURL(getWindowText(IDC_URL)));
}

LRESULT CConnectionDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CenterWindow(GetParent());

	setWindowText(serverName, IDC_NAME);
	setWindowText(serverUrl, IDC_URL);
	setWindowText(userName, IDC_LOGIN);
	setWindowText(userPassword, IDC_PASSWORD);

	{
		LOGFONT lf = { 0 };
		CFontHandle{ GetFont() }.GetLogFont(lf);
		wcscpy(lf.lfFaceName, L"FontAwesome");
		lf.lfHeight *= 38;
		lf.lfHeight /= 8;
		m_symbols.CreateFontIndirect(&lf);
	}

	SendDlgItemMessage(IDC_STATIC_SERVER, (WPARAM)(HFONT)m_symbols, TRUE);
	SendDlgItemMessage(IDC_STATIC_USER,   (WPARAM)(HFONT)m_symbols, TRUE);
	SetDlgItemText(IDC_STATIC_SERVER, L"\xf233");
	SetDlgItemText(IDC_STATIC_USER, L"\xf007");

	updateExitState();

	return TRUE;
}

LRESULT CConnectionDlg::OnTextChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	updateExitState();
	return 0;
}

LRESULT CConnectionDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if (wID == IDOK) {
		serverName = getWindowText(IDC_NAME);
		serverUrl = getWindowText(IDC_URL);
		userName = getWindowText(IDC_LOGIN);
		userPassword = getWindowText(IDC_PASSWORD);
	}

	EndDialog(wID);
	return 0;
}
