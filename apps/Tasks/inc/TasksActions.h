#pragma once

#include <vector>
#include <gui/menu.hpp>
#include <map>

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
	std::shared_ptr<gui::action> tasks_exit;

	std::shared_ptr<gui::action> conn_edit;
	std::shared_ptr<gui::action> conn_refresh;
	std::shared_ptr<gui::action> conn_remove;
	std::shared_ptr<gui::action> conn_goto;
	std::shared_ptr<gui::action> conn_logwork;

	std::shared_ptr<gui::action> help_licences;
	std::shared_ptr<gui::action> help_about;

	std::shared_ptr<gui::icon> icon_taskbar;
	std::shared_ptr<gui::icon> icon_frame;

	bool onCommand(menu::command_id cmd);
	HMENU createMenuBar(const std::initializer_list<menu::item>& items);
	HWND createToolbar(const std::initializer_list<menu::item>& items, HWND hWndParent, DWORD dwStyle = ATL_SIMPLE_TOOLBAR_STYLE, UINT nID = ATL_IDW_TOOLBAR);
};

template <typename T>
struct CTasksActions : CTasksActionsBase {

	void createItems() {
		std::shared_ptr<gui::icon> ico_none;
		auto ico_new_file = gui::make_fa_icon({ { fa::glyph::file,    0xFFFFFF       }, { fa::glyph::file_o                          } });
		auto ico_refresh  = gui::make_fa_icon({ { fa::glyph::refresh                 }                                                 });
		auto ico_setup    = gui::make_fa_icon({ { fa::glyph::wrench,  0x444444       }                                                 });
		auto ico_edit     = gui::make_fa_icon({ { fa::glyph::pencil                  }                                                 });
		auto ico_link     = gui::make_fa_icon({ { fa::glyph::chain,   0x419641, 5, 4 }                                                 });
		auto ico_licences = gui::make_fa_icon({ { fa::glyph::bank,    0x444444, 5, 4 }                                                 });
		auto ico_about    = gui::make_fa_icon({ { fa::glyph::circle,  0xFFFFFF, 3, 2 }, { fa::glyph::question_circle, 0x428BCA, 3, 2 } });

		icon_taskbar      = gui::make_fa_icon({ { fa::glyph::stop,    0x000000, 3, 2 }, { fa::glyph::tasks,           0xFFFFFF       } });
		icon_frame        = gui::make_fa_icon({ { fa::glyph::stop,    0xFFFFFF, 3, 2 }, { fa::glyph::tasks,                          } });

		auto pThis = static_cast<T*>(this);

		toolbar_default = gui::make_action(ico_none,   "Show/hide Tasks",    {},                         {},               [pThis] { pThis->showHide(); });
		tasks_new     = gui::make_action(ico_new_file, "New &Connection...", { modifier::ctrl, vk::N },  "New Connection", [pThis] { pThis->newConnection(); });
		tasks_refresh = gui::make_action(ico_refresh,  "&Refresh All",       { vk::F5 },                 "Refresh All",    [pThis] { pThis->refreshAll(); });
		tasks_setup   = gui::make_action(ico_setup,    "&Settings...",       { },                        "Settings");
		tasks_exit    = gui::make_action(ico_none,     "E&xit",              { modifier::ctrl, vk::F4 }, "Exit",           [pThis] { pThis->exitApplication(); });

		conn_edit     = gui::make_action(ico_edit,     "&Edit...");
		conn_refresh  = gui::make_action(ico_none,     "&Refresh");
		conn_remove   = gui::make_action(ico_none,     "Re&move");
		conn_goto     = gui::make_action(ico_link,     "&Go To Issue");
		conn_logwork  = gui::make_action(ico_none,     "Log &Work...");

		help_licences = gui::make_action(ico_licences, "Show &Licences",    {},                          "Licences",       [pThis] { pThis->showLicence(); });
		help_about    = gui::make_action(ico_about,    "&About Tasks...",   {},                          "About Tasks",    [pThis] { pThis->about(); });

		pThis->SetMenu(createMenuBar({
			menu::submenu(gui::make_action({}, "&Tasks"),{
				tasks_new,
				menu::separator(),
				tasks_refresh,
				tasks_setup,
				menu::separator(),
				tasks_exit,
			}),

			menu::submenu(gui::make_action({}, "&Connections"),{
				conn_edit,
				conn_refresh,
				conn_remove,
				menu::separator(),
				conn_goto,
				conn_logwork
			}),

			menu::submenu(gui::make_action({}, "&Help"),{
				help_licences,
				menu::separator(),
				help_about,
			}),
		}));

		pThis->m_hWndToolBar = createToolbar({
			tasks_new,
			menu::separator(),
			tasks_refresh,
			tasks_setup,
			menu::separator(),
			help_licences,
			help_about,
		}, pThis->m_hWnd);
	};

	void showHide() {}
	void newConnection() {}
	void refreshAll() {}
	void exitApplication() {}
	void showLicence() {}
	void about() {}
};
