#pragma once

#include <vector>
#include <gui/menu.hpp>
#include <map>
#include "langs.h"

class Win32Manager : public menu::ui_manager {
	std::map<menu::command_id, std::shared_ptr<gui::action>> m_knownActions;
	menu::command_id m_lastId;
public:
	Win32Manager(menu::command_id firstId);
	menu::command_id append(const std::shared_ptr<gui::action>& action) override;
	std::shared_ptr<gui::action> find(menu::command_id cmd) override;
	HACCEL createAccel();
	void clear();
};

struct CTasksActionsBase {
	Win32Manager m_menubarManager{ 32782 }; // _APS_NEXT_COMMAND_VALUE at the time of writing this text

	std::shared_ptr<gui::action> toolbar_default;
	std::shared_ptr<gui::action> tasks_new;
	std::shared_ptr<gui::action> tasks_refresh;
	std::shared_ptr<gui::action> tasks_setup;
	std::shared_ptr<gui::action> tasks_lang_sys;
	std::vector<menu::item> tasks_langs;
	std::shared_ptr<gui::action> tasks_exit;

	std::shared_ptr<gui::action> conn_edit;
	std::shared_ptr<gui::action> conn_refresh;
	std::shared_ptr<gui::action> conn_remove;
	std::shared_ptr<gui::action> conn_goto;
	std::shared_ptr<gui::action> conn_logwork;

	std::shared_ptr<gui::action> help_licences;
	std::shared_ptr<gui::action> help_about;

	std::shared_ptr<gui::icon> ico_new_file;
	std::shared_ptr<gui::icon> ico_refresh;
	std::shared_ptr<gui::icon> ico_setup;
	std::shared_ptr<gui::icon> ico_edit;
	std::shared_ptr<gui::icon> ico_link;
	std::shared_ptr<gui::icon> ico_licences;
	std::shared_ptr<gui::icon> ico_about;


	bool onCommand(menu::command_id cmd);
	HMENU createMenuBar(const std::initializer_list<menu::item>& items);
	HWND createToolbar(const std::initializer_list<menu::item>& items, HWND hWndParent, DWORD dwStyle = ATL_SIMPLE_TOOLBAR_STYLE, UINT nID = ATL_IDW_TOOLBAR);
};

struct lng_or_empty {
	lng m_lng { (lng)0 };
	lng_or_empty() {}
	lng_or_empty(lng l) : m_lng(l) {}

	std::string visit(const Strings& tr) const
	{
		if (m_lng == (lng)0)
			return { };

		return tr(m_lng);
	}
};

inline std::shared_ptr<gui::action>
make_action(const Strings& tr, const std::shared_ptr<gui::icon>& icon, const lng_or_empty& text, const hotkey& hotkey = { }, const lng_or_empty& tooltip = { }, const std::function<void()>& f = { })
{
	return gui::make_action(icon, text.visit(tr), hotkey, tooltip.visit(tr), f);
}

template <typename T>
struct CTasksActions : CTasksActionsBase {

	void createIcons()
	{
		ico_new_file = gui::make_fa_icon({ { fa::glyph::file,    0xFFFFFF       }, { fa::glyph::file_o                          } });
		ico_refresh  = gui::make_fa_icon({ { fa::glyph::refresh                 }                                                 });
		ico_setup    = gui::make_fa_icon({ { fa::glyph::wrench,  0x444444       }                                                 });
		ico_edit     = gui::make_fa_icon({ { fa::glyph::pencil                  }                                                 });
		ico_link     = gui::make_fa_icon({ { fa::glyph::chain,   0x419641, 5, 4 }                                                 });
		ico_licences = gui::make_fa_icon({ { fa::glyph::bank,    0x444444, 5, 4 }                                                 });
		ico_about    = gui::make_fa_icon({ { fa::glyph::circle,  0xFFFFFF, 3, 2 }, { fa::glyph::question_circle, 0x428BCA, 3, 2 } });
	}

	void createItems(const Strings& tr)
	{
		std::shared_ptr<gui::icon> ico_none;

		auto pThis = static_cast<T*>(this);

		toolbar_default = make_action(tr, ico_none,   lng::LNG_APP_MENUITEM_SHOWHIDE,   {},                         {},                                    [pThis] { pThis->showHide(); });
		tasks_new     = make_action(tr, ico_new_file, lng::LNG_APP_MENUITEM_NEWCONN,    { modifier::ctrl, vk::N },  lng::LNG_APP_MENUITEM_NEWCONN_TIP,     [pThis] { pThis->newConnection(); });
		tasks_refresh = make_action(tr, ico_refresh,  lng::LNG_APP_MENUITEM_REFRESHALL, { vk::F5 },                 lng::LNG_APP_MENUITEM_REFRESHALL_TIP,  [pThis] { pThis->refreshAll(); });
		tasks_setup   = make_action(tr, ico_setup,    lng::LNG_APP_MENUITEM_SETTINGS,   {},                         lng::LNG_APP_MENUITEM_SETTINGS_TIP);
		tasks_exit    = make_action(tr, ico_none,     lng::LNG_APP_MENUITEM_EXIT,       { modifier::ctrl, vk::F4 }, lng::LNG_APP_MENUITEM_EXIT_TIP,        [pThis] { pThis->exitApplication(); });

		conn_edit     = make_action(tr, ico_edit,     lng::LNG_APP_MENUITEM_EDIT);
		conn_refresh  = make_action(tr, ico_none,     lng::LNG_APP_MENUITEM_REFRESH);
		conn_remove   = make_action(tr, ico_none,     lng::LNG_APP_MENUITEM_REMOVE);
		conn_goto     = make_action(tr, ico_link,     lng::LNG_APP_MENUITEM_GOTO);
		conn_logwork  = make_action(tr, ico_none,     lng::LNG_APP_MENUITEM_LOGWORK);

		help_licences = make_action(tr, ico_licences, lng::LNG_APP_MENUITEM_LICENSES,   {},                          lng::LNG_APP_MENUITEM_LICENSES_TIP,   [pThis] { pThis->showLicence(); });
		help_about    = make_action(tr, ico_about,    lng::LNG_APP_MENUITEM_ABOUTTASKS, {},                          lng::LNG_APP_MENUITEM_ABOUTTASKS_TIP, [pThis] { pThis->about(); });

		{
			tasks_langs.clear();

			auto langs = tr.known();
			auto in_langs = [&](auto& cur) {
				for (auto& lang : langs) {
					if (lang.lang == cur) {
						return true;
					}
				}

				return false;
			};
			auto saved = CAppSettings { }.language();
			if (!saved.empty()) {
				if (!in_langs(saved))
					saved.clear();
			}

			auto sys_lang = [&]() -> locale::culture {
				auto sys = locale::system_locales();
				for (auto& lng : sys) {
					for (auto& lang : langs) {
						if (lang.lang == lng) {
							return lang;
						}
					}
				}

				return { };
			}();

			if (!sys_lang.lang.empty()) {
				tasks_lang_sys = gui::make_action(ico_none, tr(lng::LNG_APP_MENUITEM_SYSLANG, sys_lang.name), { }, { }, [pThis] { pThis->setLanguage(std::string { }); });
				tasks_langs.push_back(tasks_lang_sys);

				if (saved.empty())
					tasks_lang_sys->check();

				if (!langs.empty())
					tasks_langs.push_back(menu::separator());
			} else if (langs.empty()) {
				auto ptr = make_action(tr, ico_none, lng::LNG_APP_MENUITEM_MISSING);
				ptr->disable();
				tasks_langs.push_back(ptr);
			}

			int count = 1;
			for (auto& lng : langs) {
				auto code = lng.lang;
				auto text = lng.name;
				if (count < 10)
					text = str::format("&%1. %2", count, text);
				else if (count == 10)
					text = str::format("&0. %2", count, text);
				++count;
				auto ptr = gui::make_action(ico_none, text, { }, { }, [pThis, code] { pThis->setLanguage(code); });
				if (lng.lang == saved)
					ptr->check();
				tasks_langs.push_back(ptr);
			}
		}
	}

	auto createAppMenu(const Strings& tr)
	{
		return createMenuBar({
			menu::submenu(make_action(tr, {}, lng::LNG_APP_MENU_TASKS),{
				tasks_new,
				menu::separator(),
				tasks_refresh,
				tasks_setup,
				menu::submenu(make_action(tr, {}, lng::LNG_APP_MENUITEM_LANGUAGES), tasks_langs),
				menu::separator(),
				tasks_exit,
			}),

			menu::submenu(make_action(tr, {}, lng::LNG_APP_MENU_CONNNECTIONS),{
				conn_edit,
				conn_refresh,
				conn_remove,
				menu::separator(),
				conn_goto,
				conn_logwork
			}),

			menu::submenu(make_action(tr, {}, lng::LNG_APP_MENU_HELP),{
				help_licences,
				menu::separator(),
				help_about,
			}),
		});
	}

	std::initializer_list<menu::item> createAppToolbar(const Strings&)
	{
		return {
			tasks_new,
			menu::separator(),
			tasks_refresh,
			tasks_setup,
			menu::separator(),
			help_licences,
			help_about,
		};
	};

	void showHide() {}
	void newConnection() {}
	void refreshAll() {}
	void exitApplication() {}
	void showLicence() {}
	void about() {}
};
