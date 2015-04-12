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

#include <gui/painter_base.hpp>
#include <windows.h>

#define GDI_NONINT_RECT

namespace gui { namespace gdi {
	class painter
		: public gui::base::painter {
	public:
		painter(HDC dc, HBRUSH background, ratio zoom, const RECT& clip, const pixels& fontSize, const std::string& fontFamily);
		painter(HDC dc, HBRUSH background, ratio zoom, const pixels& fontSize, const std::string& fontFamily);
		~painter();

		// gui::painter
		void paintImage(const image_ref* img, const pixels& width, const pixels& height) override;
		void paintString(const std::string& text) override;
		size measureString(const std::string& text) override;
		void fillRectangle(colorref color, const point& pt, const size& size) override;
		void drawBorder(line_style style, colorref color, const gui::point& pt, const gui::size& size) override;
		void setFont(const pixels& fontSize, const std::string& fontFamily, gui::weight weight,
			bool italic, bool underline) override;
		void setColor(colorref color) override;
		colorref getColor() const override;

	private:
		void selectFont(const pixels& fontSize, const std::string& fontFamily, int weight, bool italic, bool underline);
		void fillSolidRect(const point& pt, const size& sz, const ratio& zoom, COLORREF clr);
#ifdef GDI_NONINT_RECT
		void fillSolidRect(const RECT& rect, COLORREF clr);
		void blendEdge(int x, int y, uint8_t alpha, int width, bool horiz, COLORREF clr);
#endif // GDI_NONINT_RECT

	private:
		template <typename H>
		struct selectable {
			H prev;
			H modified;
			selectable()
				: prev{ nullptr }
				, modified{ nullptr }
			{
			}

			~selectable()
			{
				if (modified)
					DeleteObject(modified);
			}

			H select(HDC dc, H value)
			{
				if (modified)
					DeleteObject(modified);

				modified = value;

				auto tmp = (H)SelectObject(dc, value);
				if (!prev)
					prev = tmp;

				return tmp;
			}

			void restore(HDC dc)
			{
				if (prev)
					SelectObject(dc, prev);
				prev = nullptr;
			}
		};

		HDC m_dc;
		selectable<HFONT> m_font;
#ifdef GDI_NONINT_RECT
		HDC m_originalDC;
		selectable<HBITMAP> m_canvas;
		uint8_t* m_pixels;
#endif // GDI_NONINT_RECT
		RECT m_clip;
	};
}};

#endif // defined(_WIN32)
