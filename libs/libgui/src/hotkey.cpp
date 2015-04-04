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
#include <gui/hotkey.hpp>
#include <cctype>

hotkey::hotkey(const modifiers& mods, vk v_key, char c_key)
	: m_mods(mods)
	, m_v_key(v_key)
	, m_c_key((char)std::toupper((uint8_t)c_key))
{
}

std::string hotkey::name() const
{
	std::string out;
	if (m_v_key == vk::undefined && m_c_key == 0)
		return{};

	modifier mods[] = { modifier::alt, modifier::ctrl, modifier::shift };
	for (auto mod : mods) {
		if (m_mods & mod) {
			auto n = modifier_name(mod);
			if (n) {
				out += n;
				out.push_back('+');
			}
		}
	}

	if (m_v_key == vk::undefined) {
		out.push_back(m_c_key);
	} else {
		auto n = key_name(m_v_key);
		if (!n || !*n)
			return{};

		out += n;
	}

	return out;
}

const char* key_name(vk key)
{
	const char* names[] = {
		nullptr,
		"Back",
		"Tab",
		"Clear",
		"Enter",
		"Pause",
		"Capital",
		"Esc",
		"Space",
		"PgUp",
		"PgDn",
		"End",
		"Home",
		"Left Arrow",
		"Up Arrow",
		"Right Arrow",
		"Down Arrow",
		"Select",
		"PrnScr",
		"Execute",
		"Snapshot",
		"Ins",
		"Del",
		"Help",
		"0",
		"1",
		"2",
		"3",
		"4",
		"5",
		"6",
		"7",
		"8",
		"9",
		"A",
		"B",
		"C",
		"D",
		"E",
		"F",
		"G",
		"H",
		"I",
		"J",
		"K",
		"L",
		"M",
		"N",
		"O",
		"P",
		"Q",
		"R",
		"S",
		"T",
		"U",
		"V",
		"W",
		"X",
		"Y",
		"Z",
		"Num 0",
		"Num 1",
		"Num 2",
		"Num 3",
		"Num 4",
		"Num 5",
		"Num 6",
		"Num 7",
		"Num 8",
		"Num 9",
		"*",
		"+",
		"Separator",
		"-",
		".",
		"/",
		"F1",
		"F2",
		"F3",
		"F4",
		"F5",
		"F6",
		"F7",
		"F8",
		"F9",
		"F10",
		"F11",
		"F12",
		"F13",
		"F14",
		"F15",
		"F16",
		"F17",
		"F18",
		"F19",
		"F20",
		"F21",
		"F22",
		"F23",
		"F24",
		"Num Lock",
		"Scroll Lock",
		"Back",
		"Forward",
		"Refresh",
		"Stop",
		"Search",
		"Favorites",
		"Home",
		"Mute",
		"Volume Down",
		"Volume Up",
		"Next Track",
		"Prev Track",
		"Stop",
		"Play/Pause",
	};

	static_assert(sizeof(names)/sizeof(names[0]) == (size_t)vk::__last,
		"Number of names does not equal the number of vk enums");

	if (key < vk::__last)
		return names[(size_t)key];

	return nullptr;
}

const char* modifier_name(modifier m)
{
	switch (m) {
	case modifier::alt: return "Alt";
	case modifier::ctrl: return "Ctrl";
	case modifier::shift: return "Shift";
	}

	return nullptr;
}
