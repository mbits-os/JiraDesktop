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
#include <gui/menu.hpp>
#include <net/utf8.hpp>

#pragma warning(push)
 // warning C4302: 'type cast' : truncation from 'LPCTSTR' to 'WORD'
 // warning C4838: conversion from 'int' to 'UINT' requires a narrowing conversion
#pragma warning(disable: 4302 4838)
#include <atlbase.h>
#include <atlapp.h>
#include <atlgdi.h>
#pragma warning(pop)

HMENU menu::item::createPopup(ui_manager* manager) const
{
	CMenu menu;
	menu.CreatePopupMenu();

	//UINT pos = 0;
	for (auto item : m_items) {
		CBitmap bmp;
		std::wstring text;

		if (item.type() == itemtype::separator) {
			menu.AppendMenu(MF_SEPARATOR);
			continue;
		}

		auto action = item.action();
		if (!action)
			continue;

		text = utf::widen(action->text());
		auto hotkey = action->hotkey().name();
		if (!hotkey.empty())
			text += utf::widen("\t" + hotkey);

		if (item.type() == itemtype::submenu) {
			CMenu sub = item.createPopup(manager);
			menu.AppendMenu(MF_STRING, sub, text.c_str());
			//sub.Detach();
		} else {
			auto id = action->id();
			auto copy = manager->find(id);
			if (copy && action == copy)
				menu.AppendMenu(MF_STRING, id, text.c_str());
			else
				menu.AppendMenu(MF_STRING, manager->append(action), text.c_str());
		}

		auto icon = action->icon();
		if (icon) {
			CBitmapHandle image = icon->getNativeBitmap(GetSystemMetrics(SM_CXSMICON));

			if (image) {
				int pos = menu.GetMenuItemCount();
				if (pos) {
					--pos;
					MENUITEMINFO mii = { 0 };
					mii.cbSize = sizeof(mii);
					mii.fMask = MIIM_BITMAP;
					mii.hbmpItem = image;

					menu.SetMenuItemInfo(pos, TRUE, &mii);
				}
			}
		}

		//bmp.Detach();
	}

	return menu.Detach();
}
