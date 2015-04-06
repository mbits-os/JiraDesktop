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
#include <gui/action.hpp>

#pragma warning(push)
// warning C4302: 'type cast' : truncation from 'LPCTSTR' to 'WORD'
// warning C4838: conversion from 'int' to 'UINT' requires a narrowing conversion
#pragma warning(disable: 4302 4838)
#include <atlbase.h>
#include <atlapp.h>
#include <atlgdi.h>
#pragma warning(pop)

namespace gui {
	class fa_icon : public icon {
		std::vector<symbol> m_stack;
		WTL::CBitmap m_calculated;
		WTL::CIcon m_icon;
		int m_size;

		struct RGBPixmap {
			WTL::CBitmap bmp;
			uint8_t* pixels = nullptr;
			LONG stride = 0;

			HBITMAP Create(HDC dc, LONG width, LONG height, WORD bpp) {
				BITMAPINFOHEADER bih = {};
				bih.biSize = sizeof(bih);
				bih.biWidth = width;
				bih.biHeight = -height;
				bih.biPlanes = 1;
				bih.biBitCount = bpp;
				bih.biCompression = BI_RGB;

				VOID* tmp = nullptr;
				auto h = bmp.CreateDIBSection(dc, reinterpret_cast<PBITMAPINFO>(&bih), DIB_RGB_COLORS, &tmp, nullptr, 0);

				if (h && tmp) {
					stride = ((width * (bpp / 8) + 3) >> 2) << 2;
					pixels = reinterpret_cast<uint8_t*>(tmp);
				}
				return h;
			}
		};

		static uint8_t clamp(int v) { return v > 0xFF ? 0xFF : v < 0 ? 0 : (uint8_t)v; }
		static uint8_t premul(uint8_t alpha, uint8_t channel) { return clamp(alpha * channel / 0xFF); }

		static COLORREF premul(uint8_t alpha, BYTE r, BYTE g, BYTE b, COLORREF color)
		{
			if (!alpha) return 0x00000000;
			if (alpha == 0xFF)  return color;
			return (alpha << 24) | RGB(premul(alpha, r), premul(alpha, g), premul(alpha, b));
		}

		static void copy(RGBPixmap& bmp32, RGBPixmap& bmp24, COLORREF color, int size)
		{
			color &= 0x00FFFFFF;
			color |= 0xFF000000;
			auto r = GetRValue(color);
			auto g = GetGValue(color);
			auto b = GetBValue(color);

			for (LONG y = 0; y < size; ++y) {
				auto dest = reinterpret_cast<COLORREF*>(bmp32.pixels + y * bmp32.stride);
				auto src = bmp24.pixels + y * bmp24.stride;

				for (LONG x = 0; x < size; ++x) {
					*src++;
					auto alpha = *src++;
					*src++;
					*dest++ = premul(0xFF - alpha, r, g, b, color);
				}
			}
		}
	public:
		fa_icon(const std::initializer_list<symbol>& stack)
			: m_stack(stack)
			, m_size(0)
		{}

		HBITMAP getNativeBitmap(int size) override
		{
			if (!m_calculated || size != m_size)
				buildBitmap(size);

			return m_calculated;
		}

		HICON getNativeIcon(int size) override
		{
			if (!m_icon || size != m_size) {
				if (!getNativeBitmap(size))
					return nullptr;

				if (m_icon)
					m_icon.DestroyIcon();

				CBitmap mono;
				mono.CreateBitmap(size, size, 1, 1, NULL);

				ICONINFO ii;
				ii.fIcon = FALSE; // Change fIcon to TRUE to create an alpha icon
				ii.xHotspot = 0;
				ii.yHotspot = 0;
				ii.hbmMask = mono;
				ii.hbmColor = m_calculated;

				m_icon.CreateIconIndirect(&ii);
			}

			return m_icon;
		}

		HICON detachIcon() override
		{
			return m_icon.Detach();
		}

		void buildBitmap(int size)
		{
			if (m_calculated)
				m_calculated.DeleteObject();

			if (m_icon)
				m_icon.DestroyIcon();

			m_size = size;

			RGBPixmap image;

			CDC display;
			display.CreateDC(L"DISPLAY", nullptr, nullptr, nullptr);

			if (!image.Create(display, size, size, 32))
				return;
			memset(image.pixels, 0x00, size * image.stride);

			CDC dc, dst;
			dc.CreateCompatibleDC(display);
			dst.CreateCompatibleDC(display);
			auto prev = dst.SelectBitmap(image.bmp);

			CFont fa;
			LOGFONT lf = {};
			fa.CreateFont( // 16 -> 12
				size * 3 / 4, 0, 0, 0, FW_NORMAL,
				FALSE, FALSE, FALSE, DEFAULT_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
				CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
				L"FontAwesome");
			fa.GetLogFont(&lf);
			auto oldFont = dc.SelectFont(fa);

			for (auto& glyph : m_stack)
			{
				RGBPixmap mask;
				RGBPixmap colorGlyph;
				if (!mask.Create(display, size, size, 24))
					return;
				if (!colorGlyph.Create(display, size, size, 32))
					return;
				memset(mask.pixels, 0xFF, size * mask.stride);

				auto height = lf.lfHeight;

				if (glyph.num != 1 || glyph.denom != 1) {
					lf.lfHeight = lf.lfHeight * glyph.num / glyph.denom;
					fa.DeleteObject();
					fa.CreateFontIndirect(&lf);
					dc.SelectFont(fa);
				}

				auto oldBmp = dc.SelectBitmap(mask.bmp);
				dc.SetBkMode(OPAQUE);
				dc.SetBkColor(0xFFFFFF);
				dc.SetTextColor(0x000000);

				WCHAR text[] = { fa::glyph_char(glyph.glyph), 0 };
				SIZE sz = {};

				dc.GetTextExtent(text, 1, &sz);
				dc.TextOut((size - sz.cx) / 2, (size - sz.cy) / 2, text, 1);

				if (height != lf.lfHeight) {
					lf.lfHeight = height;
					fa.DeleteObject();
					fa.CreateFontIndirect(&lf);
					dc.SelectFont(fa);
				}

				copy(colorGlyph, mask, glyph.color, size);
				dc.SelectBitmap(colorGlyph.bmp);
				dst.AlphaBlend(0, 0, size, size, dc, 0, 0, size, size, { AC_SRC_OVER, 0, 0xFF, AC_SRC_ALPHA });

				dc.SelectBitmap(oldBmp);
			}
			dc.SelectFont(oldFont);
			dst.SelectBitmap(prev);

			m_calculated.Attach(image.bmp.Detach());
		}
	};

	std::shared_ptr<icon> make_fa_icon(const std::initializer_list<symbol>& stack)
	{
		return std::make_shared<fa_icon>(stack);
	}
}
