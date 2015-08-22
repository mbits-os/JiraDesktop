#pragma once

#include <windows.h>
#include <memory>
#include <mutex>
#include <gui/document.hpp>
#include "langs.h"

class CAppModel;
class CredentialManager : public gui::credential_ui {

	HWND m_hWnd = nullptr;
	DWORD m_boundThread = 0;
	Strings m_strings;
	std::shared_ptr<CAppModel> m_model;
	bool m_showingUI = false;

	std::mutex m_mutex;
	struct TaskConn {
		std::promise<bool> promise;
		std::shared_ptr<owner> owner;
	};
	struct Task {
		std::string key, url, realm;
		std::vector<TaskConn> conns;
	};

	std::vector<Task> m_tasks;

	static UINT pingMsg()
	{
		static UINT msg = RegisterWindowMessage(L"WM_SIGNAL_CREDENTIAL_MANAGER");
		return msg;
	}

	std::future<bool> attach(const std::shared_ptr<owner>& owner, const std::string& url, const std::string& realm);
public:
	void setHandle(HWND hwnd, const Strings& tr, const std::shared_ptr<CAppModel>& model)
	{
		m_hWnd = hwnd;
		m_boundThread = GetWindowThreadProcessId(hwnd, nullptr);
		m_strings = tr;
		m_model = model;
	}

	BEGIN_MSG_MAP(CredentialManager)
		MESSAGE_HANDLER(pingMsg(), OnSignalMessage)
	END_MSG_MAP()
	LRESULT OnSignalMessage(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	std::future<bool> ask_user(const std::shared_ptr<owner>& owner, const std::string& url, const std::string& realm) override;
};
