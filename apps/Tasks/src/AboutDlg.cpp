// aboutdlg.cpp : implementation of the CAboutDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "version.h"

#include "AboutDlg.h"
#include <string>

#define WIDE2(x) L ## x
#define WIDE(x) WIDE2(x)
#define WPROGRAM_NAME WIDE(PROGRAM_NAME)
#define WPROGRAM_VERSION_STRING WIDE(PROGRAM_VERSION_STRING)
#define WPROGRAM_VERSION_STABILITY WIDE(PROGRAM_VERSION_STABILITY)
#define WPROGRAM_VERSION_BUILD WIDE(VERSION_STRINGIFY(PROGRAM_VERSION_BUILD))
#define WPROGRAM_COPYRIGHT_HOLDER WIDE(PROGRAM_COPYRIGHT_HOLDER)

const wchar_t* fancyStability() {
	if (*WPROGRAM_VERSION_STABILITY == '-') {
		switch (WPROGRAM_VERSION_STABILITY[1]) {
		case 'a': return L"-\x03B1";
		case 'b': return L"-\x03B2";
		}
	}
	return WPROGRAM_VERSION_STABILITY;
}

LRESULT CAboutDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	std::wstring ver{ WPROGRAM_NAME L" v" WPROGRAM_VERSION_STRING L"." WPROGRAM_VERSION_BUILD };
	ver.append(fancyStability());
	ver.append(L"\n\n\xA9 2015 " WPROGRAM_COPYRIGHT_HOLDER);

	SetDlgItemText(IDC_STATIC_VERSION, ver.c_str());
	CenterWindow(GetParent());
	return TRUE;
}

LRESULT CAboutDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	EndDialog(wID);
	return 0;
}
