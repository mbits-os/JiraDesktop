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

#include <cairo/cairo.h>
#include <gui/painter.hpp>
//#include <gui/cairo_style_handle.hpp>

namespace gui { namespace cairo {
	class painter
		: public gui::painter {
	public:
		painter(cairo_surface_t* surface, ratio zoom, ratio device, const pixels& fontSize, const std::string& fontFamily);
		~painter();

		// gui::painter
		void moveOrigin(const pixels& x, const pixels& y) override;
		point getOrigin() const override;
		void setOrigin(const point& orig) override;
		void paintImage(const image_ref* img, const pixels& width, const pixels& height) override;
		void paintString(const std::string& text) override;
		size measureString(const std::string& text) override;
		int dpiRescale(int size) override;
		long double dpiRescale(long double size) override;

		bool visible(node*) const override;
		gui::style_handle applyStyle(node*) override;
		void restoreStyle(gui::style_handle) override;

		ratio gdiRatio() const { return m_device; }

	private:
		ratio m_zoom;
		ratio m_device;
		point m_origin;
		pixels m_fontSize;
		std::string m_fontFamily;
		cairo_surface_t* m_surface;
		cairo_t* m_cr;
	};
}};
