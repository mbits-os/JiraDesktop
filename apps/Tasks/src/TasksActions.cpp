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
