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
#include <gui/font_awesome.hh>
#include <memory>
#include <functional>

#ifdef _WIN32
#include <windows.h>
#endif

namespace gui {
	class icon {
	public:
		virtual ~icon() {}
#ifdef _WIN32
		virtual HBITMAP getNativeBitmap(int size) = 0;
		virtual HICON getNativeIcon(int size) = 0;
		virtual HICON detachIcon() = 0;
#endif
	};

	class action {
        enum class flag {
            none = 0,
            disabled = 1,
            checked = 2,
        };
        friend flag& operator |= (flag& lhs, flag rhs)
        {
            return (flag&)((int&)lhs |= (int)rhs);
        }
        friend flag& operator &= (flag& lhs, flag rhs)
        {
            return (flag&)((int&)lhs &= (int)rhs);
        }
        friend flag operator | (flag lhs, flag rhs)
        {
            return (flag)((int)lhs | (int)rhs);
        }
        friend flag operator & (flag lhs, flag rhs)
        {
            return (flag)((int)lhs & (int)rhs);
        }
        friend flag operator ~ (flag lhs)
        {
            return (flag)(~(int)lhs);
        }
        std::shared_ptr<icon> m_icon;
		std::string m_text;
		std::string m_tooltip;
		hotkey m_hotkey;
		std::function<void()> m_fn;
		uint16_t m_osId = 0;
        flag m_flags = flag::none;
	public:
		action(const std::shared_ptr<icon>& icon, const std::string& text, const hotkey& hotkey, const std::string& tooltip, const std::function<void()>& f)
			: m_icon(icon)
			, m_text(text)
			, m_tooltip(tooltip)
			, m_hotkey(hotkey)
			, m_fn(f)
		{}

		const std::shared_ptr<icon>& icon() const { return m_icon; }
		const std::string& text() const { return m_text; }
		const std::string& tooltip() const { return m_tooltip; }
		const hotkey& hotkey() const { return m_hotkey; }
		uint16_t id() const { return m_osId; }
		void id(uint16_t val) { m_osId = val; }

		void call() { if (m_fn) m_fn(); }

        void check(bool set = true)
        {
            if (set)
                m_flags |= flag::checked;
            else
                m_flags &= ~flag::checked;

            // TODO: notify UI subsystem?
        }
        bool checked() const { return (m_flags & flag::checked) == flag::checked; }

        void enable(bool set = true) { disable(!set); }
        void disable(bool set = true)
        {
            if (set)
                m_flags |= flag::disabled;
            else
                m_flags &= ~flag::disabled;

            // TODO: notify UI subsystem?
        }
        bool enabled() const { return int(m_flags & flag::disabled) == 0; }
	};

	inline std::shared_ptr<action>
		make_action(const std::shared_ptr<icon>& icon, const std::string& text, const hotkey& hotkey = {}, const std::string& tooltip = {}, const std::function<void()>& f = {})
	{
		return std::make_shared<action>(icon, text, hotkey, tooltip, f);
	}

	struct symbol {
		fa::glyph glyph;
		COLORREF color;
		int num;
		int denom;

		symbol(fa::glyph glyph, COLORREF color, int num, int denom) : glyph(glyph), color(color), num(num), denom(denom) {}
		symbol(fa::glyph glyph, COLORREF color) : symbol(glyph, color, 1, 1) {}
		symbol(fa::glyph glyph) : symbol(glyph, 0x000000, 1, 1) {}
	};

	std::shared_ptr<gui::icon> make_fa_icon(const std::initializer_list<symbol>&);
};