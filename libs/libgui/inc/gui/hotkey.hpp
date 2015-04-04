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

enum class vk {
	undefined,
	back,
	tab,
	clear,
	enter,
	pause,
	capital,
	escape,
	space,
	prior,
	next,
	end,
	home,
	left,
	up,
	right,
	down,
	select,
	print,
	execute,
	snapshot,
	insert,
	del,
	help,
	num0,
	num1,
	num2,
	num3,
	num4,
	num5,
	num6,
	num7,
	num8,
	num9,
	A,
	B,
	C,
	D,
	E,
	F,
	G,
	H,
	I,
	J,
	K,
	L,
	M,
	N,
	O,
	P,
	Q,
	R,
	S,
	T,
	U,
	V,
	W,
	X,
	Y,
	Z,
	numpad0,
	numpad1,
	numpad2,
	numpad3,
	numpad4,
	numpad5,
	numpad6,
	numpad7,
	numpad8,
	numpad9,
	multiply,
	add,
	separator,
	subtract,
	decimal,
	divide,
	F1,
	F2,
	F3,
	F4,
	F5,
	F6,
	F7,
	F8,
	F9,
	F10,
	F11,
	F12,
	F13,
	F14,
	F15,
	F16,
	F17,
	F18,
	F19,
	F20,
	F21,
	F22,
	F23,
	F24,
	numlock,
	scroll,
	browser_back,
	browser_forward,
	browser_refresh,
	browser_stop,
	browser_search,
	browser_favorites,
	browser_home,
	volume_mute,
	volume_down,
	volume_up,
	media_next_track,
	media_prev_track,
	media_stop,
	media_play_pause,
	__last
};

enum class modifier {
	none  = 0,
	alt   = 1,
	ctrl  = 2,
	shift = 4
};

class modifiers {
	int m_value;
	modifiers(int val) : m_value(val) {}
public:
	modifiers() : m_value(0) {}
	modifiers(const modifiers&) = default;
	modifiers& operator=(const modifiers&) = default;

	modifiers(modifier mod) : m_value((int)mod) {}

	modifiers operator~() const { return ~m_value; }

	modifiers operator|(const modifiers& mods) const { return m_value | mods.m_value; }
	modifiers operator&(const modifiers& mods) const { return m_value & mods.m_value; }

	explicit operator bool() const { return !!m_value; }
};

inline modifiers operator~(modifier mod) { return ~modifiers(mod); }
inline modifiers operator|(modifier lhs, modifier rhs) { return modifiers(lhs) | modifiers(rhs); }
inline modifiers operator&(modifier lhs, modifier rhs) { return modifiers(lhs) & modifiers(rhs); }
inline modifiers operator|(modifier lhs, modifiers rhs) { return modifiers(lhs) | rhs; }
inline modifiers operator&(modifier lhs, modifiers rhs) { return modifiers(lhs) & rhs; }

const char* key_name(vk);
const char* modifier_name(modifier);

class hotkey {
	modifiers m_mods;
	vk m_v_key;
	char m_c_key;
	hotkey(const modifiers& mods, vk v_key, char c_key);
public:
	hotkey() : hotkey(modifier::none, vk::undefined, 0) {}
	hotkey(const modifiers& mods, vk key) : hotkey(mods, key, 0) {}
	hotkey(const modifiers& mods, char key) : hotkey(mods, vk::undefined, key) {}
	hotkey(vk key) : hotkey(modifier::none, key, 0) {}
	hotkey(char key) : hotkey(modifier::none, vk::undefined, key) {}

	modifiers mods() const { return m_mods; }
	vk v_key() const { return m_v_key; }
	char c_key() const { return m_c_key; }

	std::string name() const;
};
