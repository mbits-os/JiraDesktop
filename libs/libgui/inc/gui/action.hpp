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

#include <gui/hotkey.hpp>
#include <memory>
#include <functional>

namespace gui {
	class icon {
	public:
		virtual ~icon() {}
#ifdef _WIN32
		virtual HBITMAP getNativeBitmap(int size) = 0;
		virtual HICON getNativeIcon(int size) = 0;
#endif
	};

	class action {
		std::shared_ptr<icon> m_icon;
		std::string m_text;
		hotkey m_hotkey;
		std::function<void()> m_fn;
	public:
		action(const std::shared_ptr<icon>& icon, const std::string& text, const hotkey& hotkey, const std::function<void()>& f)
			: m_icon(icon)
			, m_text(text)
			, m_hotkey(hotkey)
			, m_fn(f)
		{}

		const std::shared_ptr<icon>& icon() const { return m_icon; }
		const std::string& text() const { return m_text; }
		const hotkey& hotkey() const { return m_hotkey; }

		void call() { if (m_fn) m_fn(); }
	};

	inline std::shared_ptr<action>
		make_action(const std::shared_ptr<icon>& icon, const std::string& text, const hotkey& hotkey = {}, const std::function<void()>& f = {})
	{
		return std::make_shared<action>(icon, text, hotkey, f);
	}

};