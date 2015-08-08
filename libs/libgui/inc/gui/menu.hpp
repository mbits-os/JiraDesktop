/*
 * Copyright (C) 2015 midnightBITS
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <gui/action.hpp>

namespace menu {
	enum class itemtype {
		action,
		separator,
		submenu
	};

	using command_id = uint16_t;

	struct ui_manager {
		virtual ~ui_manager() {}
		virtual command_id append(const std::shared_ptr<gui::action>& action) = 0;
		virtual std::shared_ptr<gui::action> find(command_id cmd) = 0;
	};

	class item {
		itemtype m_type;
		std::shared_ptr<gui::action> m_action;
		std::vector<item> m_items;
	public:
		item(const std::shared_ptr<gui::action>& action)
			: m_type(itemtype::action)
			, m_action(action)
		{}
		item(const std::shared_ptr<gui::action>& action, const std::initializer_list<item>& items)
			: m_type(itemtype::submenu)
			, m_action(action)
			, m_items(items)
		{}
		item(const std::shared_ptr<gui::action>& action, const std::vector<item>& items)
			: m_type(itemtype::submenu)
			, m_action(action)
			, m_items(items)
		{}
		item()
			: m_type(itemtype::separator)
		{}

		itemtype type() const { return m_type; }
		const std::shared_ptr<gui::action>& action() const { return m_action; }
		const std::vector<item>& items() const { return m_items; }

#ifdef _WIN32
		HMENU createPopup(ui_manager* manager) const;
#endif
	};

	inline item menu(const std::initializer_list<item>& items) { return{ nullptr, items }; }
	inline item popup(const std::initializer_list<item>& items) { return{ nullptr, items }; }
	inline item submenu(const std::shared_ptr<gui::action>& action, const std::initializer_list<item>& items) { return{ action, items }; }
	inline item submenu(const std::shared_ptr<gui::action>& action, const std::vector<item>& items) { return{ action, items }; }
	inline item separator() { return{}; }
};
