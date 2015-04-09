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
#include <gui/painter_base.hpp>
#include <gui/node.hpp>
#include <net/utf8.hpp>

#include <assert.h>
#define ASSERT(x) assert(x)

namespace gui { namespace base {

	painter::painter(ratio zoom, const pixels& fontSize, const std::string& fontFamily)
		: m_zoom{ zoom.num, zoom.denom }
		, m_origin{ 0, 0 }
		, m_fontSize{ fontSize }
		, m_fontFamily{ fontFamily }
		, m_weight{ weight::normal }
		, m_italic{ false }
		, m_underline{ false }
	{
	}

	painter::~painter()
	{
	}

	void painter::moveOrigin(const pixels& x, const pixels& y)
	{
		m_origin.x += x;
		m_origin.y += y;
	}

	point painter::getOrigin() const
	{
		return m_origin;
	}

	void painter::setOrigin(const point& orig)
	{
		m_origin = orig;
	}

	void painter::paintBackground(colorref color, const pixels& width, const pixels& height)
	{
		fillRectangle(color, {}, { width, height });
	}

	struct Border {
		pixels width;
		line_style style;
		colorref color;

		Border(const styles::rule_storage& styles,
			styles::length_prop width_prop,
			styles::border_style_prop style_prop,
			styles::color_prop color_prop)
			: width(0)
			, style(line_style::none)
			, color(0x000000)
		{
			if (styles.has(width_prop)) {
				auto u = styles.get(width_prop);
				ASSERT(u.which() == styles::length_u::first_type);
				width = u.first().value();
			}

			if (styles.has(style_prop))
				style = styles.get(style_prop);

			if (styles.has(color_prop))
				color = styles.get(color_prop);

			if (width.value() == 0 ||
				style == line_style::none)
			{
				width = pixels();
				style = line_style::none;
			}
		};

		bool present() const { return style != line_style::none; }
	};

	void painter::paintBorder(gui::node* node)
	{
		auto styles = node->calculatedStyle();
#define BORDER_(side) \
Border border_ ## side{*styles, \
	styles::prop_border_ ## side ## _width, \
	styles::prop_border_ ## side ## _style, \
	styles::prop_border_ ## side ## _color};
		MAKE_FOURWAY(BORDER_)
#undef BORDER_

		point pt {};
		auto sz = node->getSize();

		if (border_top.present()) {
			auto orig = pt;
			auto dest = pt + size{ sz.width, border_top.width };

			if (border_right.present())
				dest.x -= border_right.width;

			drawBorder(border_top.style, border_top.color, orig, dest - orig);
		}

		if (border_right.present()) {
			auto orig = pt;
			auto dest = pt + size{ sz.width, sz.height };
			orig.x = dest.x - border_right.width;

			if (border_bottom.present())
				dest.y -= border_bottom.width;

			drawBorder(border_right.style, border_right.color, orig, dest - orig);
		}

		if (border_bottom.present()) {
			auto orig = pt;
			auto dest = pt + size{ sz.width, sz.height };
			orig.y = dest.y - border_bottom.width;

			if (border_left.present())
				orig.x += border_left.width;

			drawBorder(border_bottom.style, border_bottom.color, orig, dest - orig);
		}

		if (border_left.present()) {
			auto orig = pt;
			auto dest = pt + size{ border_left.width, sz.height };

			if (border_top.present())
				orig.y += border_top.width;

			drawBorder(border_left.style, border_left.color, orig, dest - orig);
		}
	}

	bool painter::visible(node* node) const
	{
		return true;
	}

	struct saved_state : style_save {
		colorref color;
		pixels fontSize;
		std::string fontFamily;
		gui::weight weight;
		bool italic;
		bool underline;
	};

	// See <http://www.w3.org/TR/CSS2/fonts.html#propdef-font-weight>
	static weight bolderThan(weight inherited)
	{
		switch (inherited) {
		case weight::bolder:
		case weight::lighter:
			ASSERT(false);
			break;
		case weight::w100:
		case weight::w200:
		case weight::w300:
			return weight::normal;

		case weight::normal:
		case weight::w400:
		case weight::w500:
			return weight::bold;

		case weight::w600:
		case weight::bold:
		case weight::w700:
		case weight::w800:
		case weight::w900:
			return weight::w900;
		}
		return weight::bold;
	}

	// See <http://www.w3.org/TR/CSS2/fonts.html#propdef-font-weight>
	static weight lighterThan(weight inherited)
	{
		switch (inherited) {
		case weight::bolder:
		case weight::lighter:
			ASSERT(false);
			break;
		case weight::w100:
		case weight::w200:
		case weight::w300:
		case weight::w400:
		case weight::normal:
		case weight::w500:
			return weight::w100;
		case weight::w600:
		case weight::w700:
		case weight::bold:
			return weight::normal;
		case weight::w800:
		case weight::w900:
			return weight::bold;
		}

		return weight::normal;
	}

	static weight abs(weight val)
	{
		switch (val) {
		case weight::normal:
			return weight::w400;
		case weight::bold:
			return weight::w700;
		default:
			break;
		};

		return val;
	}

	static weight lighterBolder(weight newVal, weight oldVal)
	{
		switch (newVal) {
		case weight::bolder: return bolderThan(oldVal);
		case weight::lighter: return lighterThan(oldVal);
		default:
			break;
		}

		return newVal;
	}

	style_handle painter::applyStyle(node* node)
	{
		auto state = std::make_unique<saved_state>();
		m_color = getColor();
		state->color = m_color;
		state->fontSize = m_fontSize;
		state->fontFamily = m_fontFamily;
		state->weight = m_weight;
		state->italic = m_italic;
		state->underline = m_underline;

		// -------------------------------------------
		auto style = node->calculatedStyle();
		auto& ref = *style;
		using namespace styles;

		if (ref.has(prop_color))
			m_color = ref.get(prop_color);
		if (ref.has(prop_italic))
			m_italic = ref.get(prop_italic);
		if (ref.has(prop_underline))
			m_underline = ref.get(prop_underline);
		if (ref.has(prop_font_weight))
			m_weight = lighterBolder(ref.get(prop_font_weight), m_weight);
		if (ref.has(prop_font_family))
			m_fontFamily = ref.get(prop_font_family);
		if (ref.has(prop_font_size)) {
			auto u = ref.get(prop_font_size);
			ASSERT(u.which() == length_u::first_type);
			m_fontSize = u.first();
		}
		// -------------------------------------------

		if (m_color != state->color)
			setColor(m_color);

		if (state->fontFamily != m_fontFamily ||
			abs(state->weight) != abs(m_weight) ||
			state->italic != m_italic ||
			state->underline != m_underline ||
			state->fontSize != m_fontSize)
		{
			setFont(m_fontSize, m_fontFamily, m_weight, m_italic, m_underline);
		}

		return state.release();
	}

	void painter::restoreStyle(style_handle saved)
	{
		std::unique_ptr<saved_state> state{
			static_cast<saved_state*>(saved)
		};

		if (!state)
			return;

		auto restoreColor = m_color != state->color;

		auto restoreFont = state->fontFamily != m_fontFamily ||
			abs(state->weight) != abs(m_weight) ||
			state->italic != m_italic ||
			state->underline != m_underline ||
			state->fontSize != m_fontSize;

		m_color = state->color;
		m_fontSize = state->fontSize;
		m_fontFamily = state->fontFamily;
		m_weight = state->weight;
		m_italic = state->italic;
		m_underline = state->underline;

		if (restoreColor)
			setColor(m_color);

		if (restoreFont)
			setFont(m_fontSize, m_fontFamily, m_weight, m_italic, m_underline);
	}
}};
