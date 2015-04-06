#include "stdafx.h"
#include "resource.h"

#include "TasksActions.h"
#include <net/utf8.hpp>
#include <gui/menu.hpp>
#include <map>

Win32Manager::Win32Manager(menu::command_id firstId)
	: m_lastId(firstId - 1)
{
}

menu::command_id Win32Manager::append(const std::shared_ptr<gui::action>& action)
{
	auto cmd = ++m_lastId;
	action->id(cmd);
	m_knownActions[cmd] = action;
	return m_lastId;
}

std::shared_ptr<gui::action> Win32Manager::find(menu::command_id cmd)
{
	auto it = m_knownActions.find(cmd);
	if (it == m_knownActions.end())
		return{};

	return it->second;
}

HACCEL Win32Manager::createAccel()
{
	std::vector<ACCEL> accels;

	for (auto& pair : m_knownActions) {
		auto hk = pair.second->hotkey();
		if (hk == hotkey())
			continue;

		ACCEL acc;
		acc.fVirt = 0;
		bool virt = hk.v_key() != vk::undefined;
		if (virt) {
			acc.fVirt |= FVIRTKEY;
			acc.key = vk2win32(hk.v_key());
		}
		else {
			acc.key = hk.c_key();
		}

		acc.fVirt |= mods2win32(hk.mods());
		acc.cmd = pair.first;

		accels.push_back(acc);
	}

	if (accels.empty())
		return nullptr;

	return CreateAcceleratorTable(&accels[0], accels.size());
}

void Win32Manager::clear()
{
	m_knownActions.clear();
}

bool CTasksActionsBase::onCommand(menu::command_id cmd) {
	auto action = m_menubarManager.find(cmd);
	if (action) {
		action->call();
		return true;
	}

	return false;
}

HMENU CTasksActionsBase::createMenuBar(const std::initializer_list<menu::item>& items)
{
	CMenu menu;
	menu.CreateMenu();

	m_menubarManager.clear();

	for (auto& item : items) {
		if (item.type() != menu::itemtype::submenu)
			continue;

		CMenu sub = item.createPopup(&m_menubarManager);
		std::wstring text;
		auto action = item.action();
		if (action)
			text = utf::widen(action->text());
		else
			text = L"?";

		menu.AppendMenu(0, sub.Detach(), text.c_str());
	}

	return menu.Detach();
}

HWND CTasksActionsBase::createToolbar(const std::initializer_list<menu::item>& items, HWND hWndParent, DWORD dwStyle, UINT nID)
{
	HINSTANCE hInst = ModuleHelper::GetResourceInstance();
	size_t count = items.size();
	auto btns = std::make_unique<TBBUTTON[]>(count);

	const int cxSeparator = 8;
	int nBmp = 0;

	auto cx = GetSystemMetrics(SM_CXSMICON);
	HIMAGELIST hImageList = ImageList_Create(cx, cx, ILC_COLOR32, 1, 5);
	ATLASSERT(hImageList != nullptr);

	auto dest = btns.get();
	for (auto& item : items) {
		auto& btn = *dest++;

		if (item.type() == menu::itemtype::action)
		{
			auto image = item.action()->icon()->getNativeBitmap(cx);

			btn.iBitmap = ImageList_Add(hImageList, image, nullptr);
			btn.idCommand = item.action()->id();
			btn.fsState = TBSTATE_ENABLED;
			btn.fsStyle = BTNS_BUTTON;
			btn.dwData = 0;
			btn.iString = 0;
		}
		else if (item.type() == menu::itemtype::separator)
		{
			btn.iBitmap = cxSeparator;
			btn.idCommand = 0;
			btn.fsState = 0;
			btn.fsStyle = BTNS_SEP;
			btn.dwData = 0;
			btn.iString = 0;
		} else
			--count;
	}

	HWND hWnd = ::CreateWindowEx(0, TOOLBARCLASSNAME, nullptr, dwStyle, 0, 0, 100, 100, hWndParent, (HMENU)LongToHandle(nID), ModuleHelper::GetModuleInstance(), nullptr);
	if (hWnd == nullptr)
	{
		ATLASSERT(FALSE);
		return nullptr;
	}

	::SendMessage(hWnd, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0L);
	::SendMessage(hWnd, TB_SETIMAGELIST, 0, (LPARAM)hImageList);
	::SendMessage(hWnd, TB_ADDBUTTONS, count, (LPARAM)btns.get());
	::SendMessage(hWnd, TB_SETBITMAPSIZE, 0, MAKELONG(cx, cx));
	const int cxyButtonMargin = 7;
	::SendMessage(hWnd, TB_SETBUTTONSIZE, 0, MAKELONG(cx + cxyButtonMargin, cx + cxyButtonMargin));

	return hWnd;
}
