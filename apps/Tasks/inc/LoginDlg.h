// aboutdlg.h : interface of the CAboutDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>
#include <net/post_mortem.hpp>
#include "langs.h"

class CLoginDlg : public CDialogImpl<CLoginDlg>
{
	void setWindowText(const std::string&, int);
	std::string getWindowText(int);
	bool hasText(int);

	CFont m_symbols;
	locale::Translation<Strings> _;
public:
	CLoginDlg(const Strings& tr)
	{
		_.tr = tr;
	}

	enum { IDD = IDD_CREDENTIALS };

	BEGIN_MSG_MAP_POSTMORTEM(CLoginDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP_POSTMORTEM()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	std::string serverUrl;
	std::string serverRealm;
	std::string userName;
	std::string userPassword;
};
