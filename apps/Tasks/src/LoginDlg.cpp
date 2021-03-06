// aboutdlg.cpp : implementation of the CAboutDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "LoginDlg.h"
#include <net/utf8.hpp>
#include <net/uri.hpp>
#include <atlstr.h>
#include <locale/format.hpp>

void CLoginDlg::setWindowText(const std::string& value, int id)
{
	SetDlgItemText(id, u2w(utf::widen(value).c_str()));
}

std::string CLoginDlg::getWindowText(int id)
{
	ATL::CString text;
	GetDlgItemText(id, text);
	return utf::narrowed({ (const char16_t*)(LPCWSTR)text, (size_t)text.GetLength() });
}

bool CLoginDlg::hasText(int id)
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

LRESULT CLoginDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	auto updateStrings = [this] {
		SetWindowText(u2w(utf::widen(_.tr(lng::LNG_APP_LOGIN_TITLE)).c_str()));
		setWindowText(_.tr(lng::LNG_APP_LOGIN_MESSAGE, serverUrl, serverRealm), IDC_STATIC_MESSAGE);
		setWindowText(_.tr(lng::LNG_APP_LOGIN_USERNAME), IDC_STATIC_LOGIN);
		setWindowText(_.tr(lng::LNG_APP_LOGIN_PASSPHRASE), IDC_STATIC_PASSWORD);
		setWindowText(_.tr(lng::LNG_APP_LOGIN_OK), IDOK);
		setWindowText(_.tr(lng::LNG_APP_LOGIN_CANCEL), IDCANCEL);
	};
	updateStrings();
	_.onupdate(updateStrings);

	CenterWindow(GetParent());

	setWindowText(userName, IDC_LOGIN);
	setWindowText(userPassword, IDC_PASSWORD);

	{
		LOGFONT lf = { 0 };
		CFontHandle{ GetFont() }.GetLogFont(lf);
		lf.lfHeight *= 38;
		lf.lfHeight /= 8;
		m_symbols.CreateFont(lf.lfHeight, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
			L"FontAwesome");
	}

	SendDlgItemMessage(IDC_STATIC_USER,   (WPARAM)(HFONT)m_symbols, TRUE);
	SetDlgItemText(IDC_STATIC_USER, L"\xf007");

	return TRUE;
}

LRESULT CLoginDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if (wID == IDOK) {
		userName = getWindowText(IDC_LOGIN);
		userPassword = getWindowText(IDC_PASSWORD);
	}

	EndDialog(wID);
	return 0;
}
