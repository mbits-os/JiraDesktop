// aboutdlg.h : interface of the CAboutDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>
#include <net/post_mortem.hpp>
#include <CredentialManager.h>
#include "langs.h"
#include <atomic>

enum {
	UM_SERVER_INFO = WM_USER + 0x100
};

class CConnectionDlg : public CDialogImpl<CConnectionDlg>
{
	void setWindowText(const std::string&, int);
	std::string getWindowText(int);
	bool hasText(int);
	void updateExitState();

	CFont m_symbols;
	WPARAM m_urlTestCounter = 0;
	enum TEST_STAGE {
		URL_INVALID,
		URL_VALID,
		ACTIVE
	} m_urlTestStage = URL_INVALID;
	std::atomic<bool> m_updatingUrl { false };

	std::shared_ptr<CredentialManager> m_credUI = std::make_shared<CredentialManager>();

	locale::Translation<Strings> _;
	std::shared_ptr<CAppModel> m_model;
public:
	CConnectionDlg(const Strings& tr, const std::shared_ptr<CAppModel>& model) : m_model { model }
	{
		_.tr = tr;
	}

	enum { IDD = IDD_CONNECTION };

	BEGIN_MSG_MAP_POSTMORTEM(CConnectionDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(UM_SERVER_INFO, OnServerInfo)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		COMMAND_CODE_HANDLER(EN_CHANGE, OnTextChanged)
		CHAIN_MSG_MAP_MEMBER((*m_credUI))
	END_MSG_MAP_POSTMORTEM()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnServerInfo(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnTextChanged(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void testURL(std::string url);

	std::string serverName;
	std::string serverUrl;
};
