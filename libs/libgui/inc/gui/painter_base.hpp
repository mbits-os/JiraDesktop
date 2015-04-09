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

namespace gui { namespace base {
	class painter
		: public gui::painter {
	public:
		painter(ratio zoom, ratio device, const pixels& fontSize, const std::string& fontFamily);
		~painter();

		// gui::painter
		void moveOrigin(const pixels& x, const pixels& y) override;
		point getOrigin() const override;
		void setOrigin(const point& orig) override;
		void paintBackground(colorref, const pixels& width, const pixels& height) override;
		void paintBorder(node*) override;

		bool visible(node*) const override;
		gui::style_handle applyStyle(node*) override;
		void restoreStyle(gui::style_handle) override;

		ratio trueZoom() const override { return zoom(); }

		virtual void fillRectangle(colorref color, const point& pt, const size& size) = 0;
		virtual void drawBorder(line_style style, colorref color, const gui::point& pt, const gui::size& size) = 0;
		virtual void setFont(const pixels& fontSize, const std::string& fontFamily, gui::weight weight, bool italic, bool underline) = 0;
		virtual void setColor(colorref color) = 0;
		virtual colorref getColor() const = 0;

	protected:
		const ratio& zoom() const { return m_zoom; }
		const point& origin() const { return m_origin; }

	private:
		ratio m_zoom;
		ratio m_device;
		point m_origin;
		// Font:
		pixels m_fontSize;
		std::string m_fontFamily;
		weight m_weight;
		bool m_italic;
		bool m_underline;
		// Other:
		colorref m_color;
	};
}};
