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

#if defined(_WIN32)

#include <gui/painter.hpp>
#include <gui/gdi_style_handle.hpp>
#include <windows.h>

namespace gui { namespace gdi {
	class painter
		: public gui::painter
		, private gui::gdi::style_save::callback {
	public:
		painter(HDC dc, const RECT& clip, HFONT font);
		painter(HDC dc, HFONT font);
		~painter();

		// gui::painter
		void moveOrigin(int x, int y) override;
		point getOrigin() const override;
		void setOrigin(const point& orig) override;
		void paintImage(const image_ref* img, size_t width, size_t height) override;
		void paintString(const std::string& text) override;
		size measureString(const std::string& text) override;
		int dpiRescale(int size) override;
		long double dpiRescale(long double size) override;

		bool visible(node*) const override;
		gui::style_handle applyStyle(node*) override;
		void restoreStyle(gui::style_handle) override;

	private:
		void drawBackground(gui::node*, styles::colorref) override;
		void drawBorder(gui::node* node) override;
		gui::painter* getPainter() override;
		COLORREF getColor() const override;
		const LOGFONT& getFont() const override;
		void setColor(COLORREF) override;
		void setFont(const LOGFONT&) override;

		void drawBorder(const gui::point& from, const gui::point& to, styles::line style, COLORREF color);

	private:
		HDC m_dc;
		HFONT m_font;
		HFONT m_original;
		HFONT m_modified;
		LOGFONT m_lf;
		point m_origin;
		RECT m_clip;
	};
}};

#endif // defined(_WIN32)
