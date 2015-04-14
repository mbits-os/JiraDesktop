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

#include "pch.h"
#include <gui/gdi_painter.hpp>
#include <gui/image.hpp>
#include <gui/node.hpp>
#include <net/utf8.hpp>
#include <cmath>

#include <assert.h>
#define ASSERT(x) assert(x)

#pragma warning(push)
 // warning C4458: declaration of 'name' hides class member
#pragma warning(disable: 4458)
#include <gdiplus.h>
#pragma warning(pop)

#pragma comment (lib, "gdiplus.lib")

namespace gui { namespace gdi {

	painter::painter(HDC dc, HBRUSH background, ratio zoom, const RECT& clip, const pixels& fontSize, const std::string& fontFamily)
		: base::painter(zoom, fontSize, fontFamily)
		, m_dc{ dc }
		, m_originalDC{ nullptr }
		, m_pixels{ nullptr }
		, m_clip{ clip.left, clip.top, clip.right, clip.bottom }
	{
		if (m_clip.left < m_clip.right && m_clip.top < m_clip.bottom) { // non-zero clip...
			background;
			auto tmp = CreateCompatibleDC(dc);
			BITMAPINFOHEADER bih = {};
			bih.biSize = sizeof(bih);
			bih.biWidth = m_clip.right - m_clip.left;
			bih.biHeight = m_clip.top - m_clip.bottom; // or -height
			bih.biPlanes = 1;
			bih.biBitCount = 24;
			bih.biCompression = BI_RGB;

			VOID* data = nullptr;
			auto bmp = CreateDIBSection(dc, reinterpret_cast<PBITMAPINFO>(&bih), DIB_RGB_COLORS, &data, nullptr, 0);
			if (bmp && data)
				m_pixels = reinterpret_cast<uint8_t*>(data);

			m_canvas.select(tmp, bmp);
			SetViewportOrgEx(tmp , -m_clip.left, -m_clip.top, nullptr);
			m_originalDC = dc;
			m_dc = tmp;

			fillSolidRect(m_clip, GetSysColor(COLOR_WINDOW));
			SetBkMode(m_dc, TRANSPARENT);
		}

		selectFont(fontSize, fontFamily, FW_NORMAL, false, false);
	}

	painter::painter(HDC dc, HBRUSH background, ratio zoom, const pixels& fontSize, const std::string& fontFamily)
		: painter(dc, background, zoom, { 0, 0, 0, 0 }, fontSize, fontFamily)
	{
	}

	painter::~painter()
	{
		if (m_originalDC) {
			::BitBlt(m_originalDC, m_clip.left, m_clip.top, m_clip.right - m_clip.left, m_clip.bottom - m_clip.top, m_dc, m_clip.left, m_clip.top, SRCCOPY);
		}

		m_font.restore(m_dc);
		m_canvas.restore(m_dc);

		if (m_originalDC) {
			::DeleteDC(m_dc);
			m_dc = m_originalDC;
		}
	}

	void painter::paintImage(const image_ref* img, const pixels& width, const pixels& height)
	{
		auto bmp = reinterpret_cast<Gdiplus::Bitmap*>(img ? img->getNativeHandle() : nullptr);
		if (img && img->getState() != gui::load_state::pixmap_available)
			bmp = nullptr;

		if (!bmp) {
			// TODO: no-image
			return;
		}

		Gdiplus::Graphics gfx{ m_dc };
		gfx.DrawImage(bmp, zoom().scaleF(origin().x), zoom().scaleF(origin().y),
			zoom().scaleF(width), zoom().scaleF(height));
	}

	void painter::paintString(const std::string& text)
	{
		if (text.empty())
			return;

		auto widen = utf::widen(text);
		::TextOut(m_dc, zoom().scaleL(origin().x), zoom().scaleL(origin().y), widen.c_str(), widen.length());
	}

	size painter::measureString(const std::string& text)
	{
		auto line = utf::widen(text);
		SIZE s = {};
		if (::GetTextExtentPoint32(m_dc, line.c_str(), line.length(), &s))
			return{ zoom().invert(s.cx), zoom().invert(s.cy) };
		return{};
	}

	pixels painter::fontBaseline()
	{
		TEXTMETRIC tm;
		if (::GetTextMetrics(m_dc, &tm))
			return zoom().invert(tm.tmAscent);
		return{};
	}

	static inline uint8_t doubleHex(uint8_t hex) { return hex /*| (hex << 4)*/; }

	void painter::fillSolidRect(const point& pt, const size& sz, const ratio& zoom, COLORREF clr)
	{
		if (m_originalDC) {
			auto left = zoom.scaleD(pt.x);
			auto top = zoom.scaleD(pt.y);
			auto right = zoom.scaleD(pt.x + sz.width);
			auto bottom = zoom.scaleD(pt.y + sz.height);

			auto c_left = std::ceil(left);
			auto c_top = std::ceil(top);
			auto f_right = std::floor(right);
			auto f_bottom = std::floor(bottom);

			if (c_left > f_right || c_top > f_bottom) {
				if (c_top > f_bottom) {
					auto width = c_left > f_right ? 1 : int(f_right - c_left);
					blendEdge((int)c_left, (int)c_top - 1, doubleHex((uint8_t)(255 * (bottom - top))), width, true, clr);
				} else {
					blendEdge((int)c_left - 1, (int)c_top, doubleHex((uint8_t)(255 * (right - left))), (int)(f_bottom - c_top), false, clr);
				}
			} else {
				// FIRST, the solid part
				RECT r{ (int)c_left, (int)c_top, (int)f_right, (int)f_bottom };
				fillSolidRect(r, clr);

				auto dleft = c_left - left;
				auto dtop = c_top - top;
				auto dright = right - f_right;
				auto dbottom = bottom - f_bottom;

				blendEdge((int)c_left, (int)c_top - 1, doubleHex((uint8_t)(255 * dtop)), (int)(f_right - c_left), true, clr);
				blendEdge((int)c_left, (int)f_bottom, doubleHex((uint8_t)(255 * dbottom)), (int)(f_right - c_left), true, clr);
				blendEdge((int)c_left - 1, (int)c_top, doubleHex((uint8_t)(255 * dleft)), (int)(f_bottom - c_top), false, clr);
				blendEdge((int)f_right, (int)c_top, doubleHex((uint8_t)(255 * dright)), (int)(f_bottom - c_top), false, clr);

				blendEdge((int)c_left - 1, (int)c_top - 1, doubleHex((uint8_t)(255 * dtop * dleft)), 1, true, clr);
				blendEdge((int)f_right, (int)c_top - 1, doubleHex((uint8_t)(255 * dtop * dright)), 1, true, clr);
				blendEdge((int)c_left - 1, (int)f_bottom, doubleHex((uint8_t)(255 * dbottom * dleft)), 1, true, clr);
				blendEdge((int)f_right, (int)f_bottom, doubleHex((uint8_t)(255 * dbottom * dright)), 1, true, clr);
			}

			return;
		}

		RECT r{ zoom.scaleL(pt.x), zoom.scaleL(pt.y),
			zoom.scaleL(pt.x + sz.width), zoom.scaleL(pt.y + sz.height) };
		COLORREF clrOld = ::SetBkColor(m_dc, clr);
		::ExtTextOut(m_dc, 0, 0, ETO_OPAQUE, &r, NULL, 0, NULL);
		::SetBkColor(m_dc, clrOld);
	}

	void painter::fillSolidRect(const RECT& rect, COLORREF clr)
	{
		auto lt_x = rect.left;
		auto lt_y = rect.top;
		auto br_x = rect.right;
		auto br_y = rect.bottom;

		if (lt_x < m_clip.left) lt_x = m_clip.left;
		if (lt_y < m_clip.top) lt_y = m_clip.top;
		if (br_x < m_clip.left) br_x = m_clip.left;
		if (br_y < m_clip.top) br_y = m_clip.top;

		if (lt_x > m_clip.right) lt_x = m_clip.right;
		if (lt_y > m_clip.bottom) lt_y = m_clip.bottom;
		if (br_x > m_clip.right) br_x = m_clip.right;
		if (br_y > m_clip.bottom) br_y = m_clip.bottom;

		lt_x -= m_clip.left;
		lt_y -= m_clip.top;
		br_x -= m_clip.left;
		br_y -= m_clip.top;

		if (lt_x >= br_x || lt_y >= br_y)
			return;

#if 1
		auto stride = (((m_clip.right - m_clip.left) * 3 + 3) >> 2) << 2;
		auto data = m_pixels + lt_y * stride + lt_x * 3;

		auto r = GetRValue(clr);
		auto g = GetGValue(clr);
		auto b = GetBValue(clr);

		for (int y = lt_y; y < br_y; ++y) {
			auto line = data;
			data += stride;

			for (long x = lt_x; x < br_x; ++x) {
				*line++ = b;
				*line++ = g;
				*line++ = r;
			}
		}
#else
		/**
		 * SCAN LINE
		 *
		 * |     A     | B |             C           |    D    |
		 * |             =======================================
		 * |      Bx     |                  Bw                 |
		 * |           |O|L|                W'                 |
		 *
		 * -------------------------------------------------------
		 *  Bx = x * 3
		 *  Bw = w * 3
		 *  A  = Bx div 4 * 4
		 *  B  = ceil(Bx / 4) * 4
		 *  O  = Bx mod 4
		 *  L  = 4 - O
		 *  W' = Bw - L
		 *  C  = W' div [chunk] * [chunk]
		 *  D  = W' - C ( = W' mod [chunk])
		 * -------------------------------------------------------
		 *
		 * LINE FILL
		 *
		 *        v START
		 * 1112|2233|3444|5556|6677|7888
		 *           ^ MASK
		 * |  L1 |L2|      L3      |SAFE
		 *
		 * -------------------------------------------------------
		 *  L3 = 12 (3 ints or 4 pixels)
		 *  L2 = 4 - L1 mod 4
		 *  L1 = f(Bx mod 4) ->
		 *        0: 0 (0th int, 0th byte) -
		 *        1: 9 (2nd int, 1st byte) ((3-1) int, (1) byte)
		 *        2: 6 (1st int, 2nd byte) ((3-2) int, (2) byte)
		 *        3: 3 (0th int, 3rd byte) ((3-3) int, (3) byte)
		 *        (3 - x) * 4 + x = 12 - 4x + x = 12 - 3x
		 *  L1 = 12 - 3 * (Bx mod 4)
		 * -------------------------------------------------------
		 */

		const auto stride = (((m_clip.right - m_clip.left) * 3 + 3) >> 2) << 2;
		auto data = m_pixels + lt_y * stride;

		const auto Bx = lt_x * 3;
		const auto Bw = (br_x - lt_x) * 3;
		const auto A = (Bx >> 2) << 2;
		const auto B = ((Bx + 3) >> 2) << 2;

		const auto offset = Bx % 4;
		const auto left = 4 - offset;

		const auto chunk = 16 * 3;
		const auto uints = chunk / sizeof(uint32_t);
		const auto Wprime = Bw - left;
		const auto C = Wprime / chunk; // number of chunk-sized blobs
		const auto D = Wprime % chunk;

		const auto L1 = 12 - 3 * (offset);
		const auto L2 = 4 - L1 % 4;

		auto r = GetRValue(clr);
		auto g = GetGValue(clr);
		auto b = GetBValue(clr);

		uint32_t stamp[2 * uints];
		auto start = ((uint8_t*)stamp);
		auto mask = start;
		size_t rest = 0;

		auto ptr = start;
		size_t count = sizeof(stamp) / 3;

		while (count--) {
			*ptr++ = b;
			*ptr++ = g;
			*ptr++ = r;
		}

		data += A;

		if (offset) {
			start += L1;
			mask = start + L2;
		}

		for (int y = lt_y; y < br_y; ++y) {
			auto line = data;
			data += stride;

			if (offset) {
				//memcpy(line, start, L2);
				line += L2;
			}

			for (long i = 0; i < C; ++i) {
				memcpy(line, mask, chunk);
				line += chunk;
			}

			memcpy(line, mask, D);
		}
#endif
	}

	void painter::blendEdge(int x, int y, uint8_t alpha, int width, bool horiz, COLORREF clr)
	{
		if (x < m_clip.left) {
			if (horiz)
				width -= m_clip.left - x;
			else
				return;
			x = m_clip.left;
		}
		if (y < m_clip.top) {
			if (!horiz)
				width -= m_clip.top - y;
			else
				return;
			y = m_clip.top;
		}

		if (x >= m_clip.right || y >= m_clip.bottom)
			return;

		if (horiz && (x + width) > m_clip.right)
			width = m_clip.right - x;
		if (!horiz && (y + width) > m_clip.bottom)
			width = m_clip.bottom - y;

		x -= m_clip.left;
		y -= m_clip.top;

		if (width <= 0)
			return;

		auto stride = (((m_clip.right - m_clip.left) * 3 + 3) >> 2) << 2;
		auto data = m_pixels + y * stride + x * 3;

		auto w = horiz ? width : 1;
		auto h = horiz ? 1 : width;

		auto r = GetRValue(clr) * alpha;
		auto g = GetGValue(clr) * alpha;
		auto b = GetBValue(clr) * alpha;
		auto alpha1 = 0xFF - alpha;

		for (int posy = 0; posy < h; ++posy) {
			auto line = data;
			data += stride;

			for (int posx = 0; posx < w; ++posx) {
				*line = (uint8_t)((b + *line * alpha1) / 0xFF); line++;
				*line = (uint8_t)((g + *line * alpha1) / 0xFF); line++;
				*line = (uint8_t)((r + *line * alpha1) / 0xFF); line++;
			}
		}
	}

	void painter::fillRectangle(colorref color, const point& pt, const size& size)
	{
		fillSolidRect(origin() + pt, size, zoom(), color);
	}

	void painter::drawBorder(line_style /*style*/, colorref color, const gui::point& pt, const gui::size& size)
	{
		fillSolidRect(origin() + pt, size, zoom(), color);
	}

	static int gui2win32(weight w)
	{
		ASSERT(w != weight::bolder);
		ASSERT(w != weight::lighter);
		switch (w) {
		case weight::normal: return FW_NORMAL;
		case weight::bold: return FW_BOLD;
		default:
			break;
		}

		return (int)w;
	}

	void painter::setFont(const pixels& fontSize, const std::string& fontFamily, gui::weight weight, bool italic, bool underline)
	{
		selectFont(fontSize, fontFamily, gui2win32(weight), italic, underline);
	}

	void painter::setColor(colorref color)
	{
		SetTextColor(m_dc, color);
	}

	colorref painter::getColor() const
	{
		return GetTextColor(m_dc);
	}

	void painter::selectFont(const pixels& fontSize, const std::string& fontFamily, int weight, bool italic, bool underline)
	{
		m_font.select(m_dc, ::CreateFont(-zoom().scaleI(fontSize), 0, 0, 0, weight, italic, underline,
			FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, utf::widen(fontFamily).c_str()));
	}
}};
