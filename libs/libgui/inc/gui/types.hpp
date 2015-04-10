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

#include <ratio>
#include <string>
#include <map>

namespace gui {
	enum class elem {
		unspecified,
		body,
		block,
		header,
		span,
		text,
		link,
		image,
		icon,
		table,
		table_head,
		table_row,
		table_caption,
		th,
		td
	};

	using colorref = uint32_t;

	enum class align {
		left,
		right,
		center
	};

	enum class line_style {
		none,
		solid,
		dot,
		dash
	};

	enum class weight {
		normal,
		bold,
		bolder,
		lighter,
		w100 = 100,
		w200 = 200,
		w300 = 300,
		w400 = 400,
		w500 = 500,
		w600 = 600,
		w700 = 700,
		w800 = 800,
		w900 = 900
	};

	enum class pointer {
		inherited,
		arrow,
		hand
	};

	enum class display {
		inlined,
		block,
		table,
		table_row,
		table_caption,
		table_header,
		table_footer,
		table_cell,
		none
	};

	template <typename Ratio>
	class length {
		long double m_len;
	public:
		using ratio = Ratio;
		length() : m_len(0.0) {};
		length(long double v) : m_len(v) {};

		long double value() const { return m_len; }

		length& operator += (const length<Ratio>& rhs)
		{
			m_len += rhs.m_len;
			return *this;
		}

		length& operator -= (const length<Ratio>& rhs)
		{
			m_len -= rhs.m_len;
			return *this;
		}
	};

	template <typename Ratio>
	length<Ratio> operator+ (const length<Ratio>& lhs, const length<Ratio>& rhs)
	{
		return lhs.value() + rhs.value();
	}

	template <typename Ratio>
	length<Ratio> operator- (const length<Ratio>& lhs, const length<Ratio>& rhs)
	{
		return lhs.value() - rhs.value();
	}

	template <typename Ratio>
	bool operator==(const length<Ratio>& lhs, const length<Ratio>& rhs)
	{
		return lhs.value() == rhs.value();
	}

	template <typename Ratio>
	bool operator!=(const length<Ratio>& lhs, const length<Ratio>& rhs)
	{
		return lhs.value() != rhs.value();
	}

	template <typename Ratio>
	bool operator<(const length<Ratio>& lhs, const length<Ratio>& rhs)
	{
		return lhs.value() < rhs.value();
	}

	template <typename Ratio>
	bool operator>(const length<Ratio>& lhs, const length<Ratio>& rhs)
	{
		return lhs.value() > rhs.value();
	}

	template <typename Ratio>
	bool operator==(const length<Ratio>& lhs, long double rhs)
	{
		return lhs.value() == rhs;
	}

	template <typename Ratio>
	bool operator!=(const length<Ratio>& lhs, long double rhs)
	{
		return lhs.value() != rhs;
	}

	template <typename Ratio>
	bool operator<(const length<Ratio>& lhs, long double rhs)
	{
		return lhs.value() < rhs;
	}

	template <typename Ratio>
	bool operator>(const length<Ratio>& lhs, long double rhs)
	{
		return lhs.value() > rhs;
	}

	using pixels = length<std::ratio<1>>;
	using points = length<std::ratio<4, 3>>;
	using inches = length<std::ratio<96>>;

	template<class To, class Ratio>
	inline To length_cast(const length<Ratio>& size)
	{
		typedef std::ratio_divide<Ratio, typename To::ratio> _CF;

		if (_CF::num == 1 && _CF::den == 1)
			return size.value();

		if (_CF::num != 1 && _CF::den == 1)
			return size.value() * _CF::num;

		if (_CF::num == 1 && _CF::den != 1)
			return size.value() / _CF::den;

		return return size.value() * _CF::num / _CF::den;
	}

	inline namespace literals {
		inline pixels operator""_px(uint64_t v) { return (long double)v; }
		inline points operator""_pt(uint64_t v) { return (long double)v; }
		inline inches operator""_in(uint64_t v) { return (long double)v; }

		inline pixels operator""_px(long double v) { return v; }
		inline points operator""_pt(long double v) { return v; }
		inline inches operator""_in(long double v) { return v; }
	};
}
