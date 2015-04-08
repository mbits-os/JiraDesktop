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
#include <gui/painter.hpp>
#include <gui/styles.hpp>
#include <windows.h>

namespace gui { namespace gdi {
	struct style_save : gui::style_save {
		struct callback {
			virtual ~callback() {}
			virtual void drawBackground(gui::node*, colorref) = 0;
			virtual void drawBorder(gui::node*) = 0;
			virtual gui::painter* getPainter() = 0;
			virtual COLORREF getColor() const = 0;
			virtual const LOGFONT& getFont() const = 0;

			virtual void setColor(COLORREF) = 0;
			virtual void setFont(const LOGFONT&) = 0;
		};

		void apply(callback*, gui::node*);
		void restore();

	private:
		callback* m_caller = nullptr;
		gui::node* m_target = nullptr;
		point m_origin;
		LOGFONT m_originalFont;
		LOGFONT m_modifiedFont;
		bool m_fontChanged;
		COLORREF m_originalTextColor;
		COLORREF m_modifiedTextColor;

		void batch_apply(styles::rule_storage&);
		void lower_layer(styles::rule_storage&);
		void move_padding(styles::rule_storage&);

		bool set_color(COLORREF);
		bool set_font_italic(bool);
		bool set_font_underline(bool);
		bool set_font_weight(int);
		bool set_font_size(const styles::length_u&);
		bool set_font_family(const std::wstring&);
		void store();
	};
}};
