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

#include "pch.h"
#include <gui/gdi_style_handle.hpp>
#include <gui/node.hpp>
#include <net/utf8.hpp>
#include <windows.h>

#include <assert.h>
#define ASSERT(x) assert(x)

namespace gui { namespace gdi {
	// See <http://www.w3.org/TR/CSS2/fonts.html#propdef-font-weight>
	static int bolderThan(int inherited)
	{
		if (inherited < FW_NORMAL)
			return FW_NORMAL;
		if (inherited > FW_SEMIBOLD)
			return FW_HEAVY;
		return FW_BOLD;
	}

	// See <http://www.w3.org/TR/CSS2/fonts.html#propdef-font-weight>
	static int lighterThan(int inherited)
	{
		if (inherited < FW_SEMIBOLD)
			return FW_THIN;
		if (inherited >= FW_EXTRABOLD)
			return FW_BOLD;
		return FW_NORMAL;
	}

	static int calc(weight w, int inherited)
	{
		switch (w) {
		case weight::normal: return FW_NORMAL;
		case weight::bold: return FW_BOLD;
		case weight::bolder: return bolderThan(inherited);
		case weight::lighter: return lighterThan(inherited);
		default:
			break;
		}

		return (int)w;
	}

	void style_save::apply(callback* cb, gui::node* node)
	{
		m_caller = cb;
		m_target = node;
		auto style = node->calculatedStyle();
		auto& ref = *style;

		m_originalFont = cb->getFont();
		m_modifiedFont = m_originalFont;
		m_fontChanged = false;

		m_originalTextColor = cb->getColor();
		m_modifiedTextColor = m_originalTextColor;

		bool update = false;
		m_fontChanged = false;

		using namespace styles;

		if (ref.has(prop_color))
			update = set_color(ref.get(prop_color));
		if (ref.has(prop_italic))
			m_fontChanged |= set_font_italic(ref.get(prop_italic));
		if (ref.has(prop_underline))
			m_fontChanged |= set_font_underline(ref.get(prop_underline));
		if (ref.has(prop_font_weight))
			m_fontChanged |= set_font_weight(calc(ref.get(prop_font_weight), m_originalFont.lfWeight));
		if (ref.has(prop_font_size))
			m_fontChanged |= set_font_size(ref.get(prop_font_size));
		if (ref.has(prop_font_family))
			m_fontChanged |= set_font_family(utf::widen(ref.get(prop_font_family)));

		if (update || m_fontChanged)
			store();
	}

	void style_save::restore()
	{
		m_caller->getPainter()->setOrigin(m_origin);

		if (m_modifiedTextColor != m_originalTextColor)
			m_caller->setColor(m_originalTextColor);

		if (m_fontChanged)
			m_caller->setFont(m_originalFont);
	}

	bool style_save::set_color(COLORREF color) {
		m_modifiedTextColor = color;
		return m_modifiedTextColor != m_originalTextColor;
	}

	bool style_save::set_font_italic(bool val)
	{
		m_modifiedFont.lfItalic = val ? TRUE : FALSE;
		return m_modifiedFont.lfItalic != m_originalFont.lfItalic;
	}

	bool style_save::set_font_underline(bool val)
	{
		m_modifiedFont.lfUnderline = val ? TRUE : FALSE;
		return m_modifiedFont.lfUnderline != m_originalFont.lfUnderline;
	}

	bool style_save::set_font_weight(int val)
	{
		m_modifiedFont.lfWeight = val;
		return m_modifiedFont.lfWeight != m_originalFont.lfWeight;
	}

	bool style_save::set_font_size(const styles::length_u& len)
	{
		if (len.which() == styles::length_u::first_type) {
			m_modifiedFont.lfHeight = 
				-(int)(m_caller->getPainter()->dpiRescale(len.first().value()) + 0.5);
		} else if (len.which() == styles::length_u::second_type) {
			m_modifiedFont.lfHeight = 
				(int)(m_originalFont.lfHeight * len.second().value() + 0.5);
		} else {
			return false;
		}

		return m_modifiedFont.lfHeight != m_originalFont.lfHeight;
	}

	bool style_save::set_font_family(const std::wstring& val)
	{
		wcscpy_s(m_modifiedFont.lfFaceName, val.c_str());

		return !!wcscmp(m_modifiedFont.lfFaceName, m_originalFont.lfFaceName);
	}

	void style_save::store()
	{
		if (m_modifiedTextColor != m_originalTextColor)
			m_caller->setColor(m_modifiedTextColor);

		if (m_fontChanged)
			m_caller->setFont(m_modifiedFont);
	}

}};
