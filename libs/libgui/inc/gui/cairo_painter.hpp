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

#if defined(HAS_CAIRO)

#include <cairo/cairo.h>
#include <gui/painter_base.hpp>
//#include <gui/cairo_style_handle.hpp>

namespace gui { namespace cairo {
	class painter
		: public gui::base::painter {
	public:
		painter(cairo_surface_t* surface, ratio zoom, const pixels& fontSize, const std::string& fontFamily);
		~painter();

		// gui::painter
		void paintImage(const image_ref* img, const pixels& width, const pixels& height) override;
		void paintString(const std::string& text) override;
		size measureString(const std::string& text) override;
		void fillRectangle(colorref color, const point& pt, const size& size) override;
		void drawBorder(line_style style, colorref color, const gui::point& pt, const gui::size& size) override;
		void setFont(const pixels& fontSize, const std::string& fontFamily, gui::weight weight, bool italic, bool underline) override;
		void setColor(colorref color) override;
		colorref getColor() const override;

	private:
		void selectFont(const pixels& fontSize, const std::string& fontFamily, int weight, bool italic, bool underline);

	private:
		cairo_surface_t* m_surface;
		cairo_t* m_cr;
		colorref m_color;
	};
}};

#endif // defined(HAS_CAIRO)
