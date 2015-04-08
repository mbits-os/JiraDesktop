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

#include <gui/types.hpp>

namespace gui {
	struct image_ref;
	struct node;
	struct style_save { int __unused; };
	using style_handle = style_save*;

	struct point {
		int x;
		int y;

		point() : x(0), y(0) {}
		point(int x, int y) : x(x), y(y) {}
		point(const point&) = default;
		point& operator=(const point&) = default;
		point(point&&) = default;
		point& operator=(point&&) = default;
	};

	struct size {
		size_t width;
		size_t height;

		size() : width(0), height(0) {}
		size(size_t width, size_t height) : width(width), height(height) {}
		size(const size&) = default;
		size& operator=(const size&) = default;
		size(size&&) = default;
		size& operator=(size&&) = default;
	};

	inline point operator + (const point& lhs, const size& rhs) {
		return{ (int)(lhs.x + rhs.width), (int)(lhs.y + rhs.height) };
	}

	struct painter {
		using point = gui::point;
		using size = gui::size;

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

		virtual bool visible(node*) const = 0;
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