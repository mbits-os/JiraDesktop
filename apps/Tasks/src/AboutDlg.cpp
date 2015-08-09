// aboutdlg.cpp : implementation of the CAboutDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "version.h"

#include "AboutDlg.h"
#include <string>
#include <format.hpp>

#define WIDE2(x) L ## x
#define WIDE(x) WIDE2(x)
#define WPROGRAM_NAME WIDE(PROGRAM_NAME)
#define WPROGRAM_VERSION_STRING WIDE(PROGRAM_VERSION_STRING)
#define WPROGRAM_VERSION_STABILITY WIDE(PROGRAM_VERSION_STABILITY)
#define WPROGRAM_VERSION_BUILD WIDE(VERSION_STRINGIFY(PROGRAM_VERSION_BUILD))
#define WPROGRAM_COPYRIGHT_HOLDER WIDE(PROGRAM_COPYRIGHT_HOLDER)

const char* fancyStability() {
	if (*PROGRAM_VERSION_STABILITY == '-') {
		switch (PROGRAM_VERSION_STABILITY[1]) {
		case 'a': return "-\xCE\xB1";
		case 'b': return "-\xCE\xB2";
		}
	}
	return PROGRAM_VERSION_STABILITY;
}

LRESULT CAboutDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	SetWindowText(utf::widen(tr(lng::LNG_APP_ABOUT_TITLE)).c_str());
	auto ver = utf::widen(
		tr(lng::LNG_APP_ABOUT_MESSAGE, tr(lng::LNG_APP_NAME), PROGRAM_VERSION_STRING, fancyStability(), PROGRAM_VERSION_BUILD, PROGRAM_COPYRIGHT_HOLDER)
		);

	SetDlgItemText(IDC_STATIC_VERSION, ver.c_str());
	CenterWindow(GetParent());
	return TRUE;
}

LRESULT CAboutDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	EndDialog(wID);
	return 0;
}
