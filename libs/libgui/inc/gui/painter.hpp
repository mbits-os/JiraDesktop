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
		pixels x;
		pixels y;

		point() : x(0), y(0) {}
		point(const pixels& x, const pixels& y) : x(x), y(y) {}
		point(const point&) = default;
		point& operator=(const point&) = default;
		point(point&&) = default;
		point& operator=(point&&) = default;

		point operator-() const {
			return{ -x, -y };
		}
	};

	struct size {
		pixels width;
		pixels height;

		size() : width(0), height(0) {}
		size(const pixels& width, const pixels& height) : width(width), height(height) {}
		size(const size&) = default;
		size& operator=(const size&) = default;
		size(size&&) = default;
		size& operator=(size&&) = default;

		bool empty() const
		{
			return !width.value() || !height.value();
		}
	};

	inline point operator + (const point& lhs, const point& rhs) {
		return{ lhs.x + rhs.x, lhs.y + rhs.y };
	}

	inline point operator + (const point& lhs, const size& rhs) {
		return{ lhs.x + rhs.width, lhs.y + rhs.height };
	}

	inline size operator - (const point& lhs, const point& rhs) {
		return{ std::abs((lhs.x - rhs.x).value()), std::abs((lhs.y - rhs.y).value()) };
	}

	struct box {
		pixels top;
		pixels right;
		pixels left;
		pixels bottom;

		box() = default;
		box(const box&) = default;
		box& operator=(const box&) = default;
		box(box&&) = default;
		box& operator=(box&&) = default;

		// this constructors takes top, right, bottom and left,
		// with values filled like in CSS
		box(const pixels& top, const pixels& right, const pixels& bottom, const pixels& left)
			: top(top)
			, right(right)
			, bottom(bottom)
			, left(left)
		{
		}

		// this constructors takes top, right and bottom,
		// with missing value filled like in CSS
		box(const pixels& top, const pixels& right, const pixels& bottom)
			: top(top)
			, right(right)
			, bottom(bottom)
			, left(right)
		{
		}

		// this constructors takes top and right,
		// with missing values filled like in CSS
		box(const pixels& top, const pixels& right)
			: top(top)
			, right(right)
			, bottom(top)
			, left(right)
		{
		}

		// this constructors takes only top,
		// with missing values filled like in CSS
		box(const pixels& top)
			: top(top)
			, right(top)
			, bottom(top)
			, left(top)
		{
		}
	};

	struct rect {
		gui::point origin;
		gui::size size;

		rect() = default;
		rect(const gui::point& topleft, const gui::point& bottomright) : origin(topleft), size(bottomright - topleft) {}
		rect(const gui::point& origin, const gui::size& size) : origin(origin), size(size) {}
		rect(const rect&) = default;
		rect& operator=(const rect&) = default;
		rect(rect&&) = default;
		rect& operator=(rect&&) = default;
	};

	struct reach : rect {
		box values;

		reach() = default;
		reach(const rect& position, const box& values) : rect(position), values(values) {}
		reach(const reach&) = default;
		reach& operator=(const reach&) = default;
		reach(reach&&) = default;
		reach& operator=(reach&&) = default;
	};

	struct css_box {
		box margin;
		box border;
		box padding;

		css_box() = default;
		css_box(const box& margin, const box& border, const box& padding) : margin(margin), border(border), padding(padding) {}
		css_box(const css_box&) = default;
		css_box& operator=(const css_box&) = default;
		css_box(css_box&&) = default;
		css_box& operator=(css_box&&) = default;
	};

	struct ratio {
		int num;
		int denom;

		long double scaleD(const pixels& px) const
		{
			return num * px.value() / denom;
		}

		float scaleF(const pixels& px) const
		{
			return (float)(num * px.value() / denom);
		}

		long scaleL(const pixels& px) const
		{
			return (long)(0.5 + num * px.value() / denom);
		}

		int scaleI(const pixels& px) const
		{
			return (int)(0.5 + num * px.value() / denom);
		}

		pixels invert(int value) const {
			return (long double)value * denom / num;
		}

		pixels invert(double value) const {
			return (long double)value * denom / num;
		}

		pixels invert(long double value) const {
			return value * denom / num;
		}

		static int GCD(int a, int b) {
			if (!b)
				return a;

			return GCD(b, a % b);
		}

		ratio gcd() const {
			auto factor = GCD(num, denom);
			return{ num / factor, denom / factor };
		}
	};

	inline ratio operator * (const ratio& lhs, const ratio& rhs)
	{
		auto out = lhs.gcd();
		auto right = rhs.gcd();

		auto gcd = ratio::GCD(out.num, right.denom);
		out.num /= gcd;
		right.denom /= gcd;

		gcd = ratio::GCD(right.num, out.denom);
		right.num /= gcd;
		out.denom /= gcd;

		out.num *= right.num;
		out.denom *= right.denom;

		return out;
	}

	struct painter {
		virtual ~painter() {}
		virtual void moveOrigin(const pixels& x, const pixels& y) = 0;
		void moveOrigin(const point& pt) { moveOrigin(pt.x, pt.y); }
		virtual point getOrigin() const = 0;
		virtual void setOrigin(const point& orig) = 0;
		virtual void paintImage(const image_ref* img, const pixels& width, const pixels& height) = 0;
		virtual void paintString(const std::string& text) = 0;
		virtual void paintBackground(colorref, const pixels& width, const pixels& height) = 0;
		virtual void paintBorder(node*) = 0;
		virtual size measureString(const std::string& text) = 0;
		virtual pixels fontBaseline() = 0;

		virtual bool visible(node*) const = 0;
		virtual style_handle applyStyle(node*) = 0;
		virtual void restoreStyle(style_handle) = 0;

		virtual ratio trueZoom() const = 0;
	};

	class push_origin {
		painter* m_painter;
		point orig;
	public:
		explicit push_origin(painter* paint) : m_painter(paint), orig(paint->getOrigin())
		{
		}

		~push_origin()
		{
			m_painter->setOrigin(orig);
		}
	};
};