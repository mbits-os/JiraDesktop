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

#include <string>

namespace gui {
	struct image_ref;
	struct node;
	struct style_save { int __unused; };
	using style_handle = style_save*;

	struct painter {
		struct point { int x; int y; };
		struct size { size_t width; size_t height; };

		virtual ~painter() {}
		virtual void moveOrigin(int x, int y) = 0;
		void moveOrigin(const point& pt) { moveOrigin(pt.x, pt.y); }
		virtual point getOrigin() const = 0;
		virtual void setOrigin(const point& orig) = 0;
		virtual void paintImage(const image_ref* img, size_t width, size_t height) = 0;
		virtual void paintString(const std::string& text) = 0;
		virtual size measureString(const std::string& text) = 0;
		virtual int dpiRescale(int size) = 0;
		virtual long double dpiRescale(long double size) = 0;

		virtual style_handle applyStyle(node*) = 0;
		virtual void restoreStyle(style_handle) = 0;
	};

	class push_origin {
		gui::painter* painter;
		gui::painter::point orig;
	public:
		explicit push_origin(gui::painter* painter) : painter(painter), orig(painter->getOrigin())
		{
		}

		~push_origin()
		{
			painter->setOrigin(orig);
		}
	};
};