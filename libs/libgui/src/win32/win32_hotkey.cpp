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
#include <windows.h>

uint16_t vk2win32(vk val)
{
	switch (val) {
	case vk::back: return VK_BACK;
	case vk::tab: return VK_TAB;
	case vk::clear: return VK_CLEAR;
	case vk::enter: return VK_RETURN;
	case vk::pause: return VK_PAUSE;
	case vk::capital: return VK_CAPITAL;
	case vk::escape: return VK_ESCAPE;
	case vk::space: return VK_SPACE;
	case vk::prior: return VK_PRIOR;
	case vk::next: return VK_NEXT;
	case vk::end: return VK_END;
	case vk::home: return VK_HOME;
	case vk::left: return VK_LEFT;
	case vk::up: return VK_UP;
	case vk::right: return VK_RIGHT;
	case vk::down: return VK_DOWN;
	case vk::select: return VK_SELECT;
	case vk::print: return VK_PRINT;
	case vk::execute: return VK_EXECUTE;
	case vk::snapshot: return VK_SNAPSHOT;
	case vk::insert: return VK_INSERT;
	case vk::del: return VK_DELETE;
	case vk::help: return VK_HELP;
	case vk::num0: return '0';
	case vk::num1: return '1';
	case vk::num2: return '2';
	case vk::num3: return '3';
	case vk::num4: return '4';
	case vk::num5: return '5';
	case vk::num6: return '6';
	case vk::num7: return '7';
	case vk::num8: return '8';
	case vk::num9: return '9';
	case vk::A: return 'A';
	case vk::B: return 'B';
	case vk::C: return 'C';
	case vk::D: return 'D';
	case vk::E: return 'E';
	case vk::F: return 'F';
	case vk::G: return 'G';
	case vk::H: return 'H';
	case vk::I: return 'I';
	case vk::J: return 'J';
	case vk::K: return 'K';
	case vk::L: return 'L';
	case vk::M: return 'M';
	case vk::N: return 'N';
	case vk::O: return 'O';
	case vk::P: return 'P';
	case vk::Q: return 'Q';
	case vk::R: return 'R';
	case vk::S: return 'S';
	case vk::T: return 'T';
	case vk::U: return 'U';
	case vk::V: return 'V';
	case vk::W: return 'W';
	case vk::X: return 'X';
	case vk::Y: return 'Y';
	case vk::Z: return 'Z';
	case vk::numpad0: return VK_NUMPAD0;
	case vk::numpad1: return VK_NUMPAD1;
	case vk::numpad2: return VK_NUMPAD2;
	case vk::numpad3: return VK_NUMPAD3;
	case vk::numpad4: return VK_NUMPAD4;
	case vk::numpad5: return VK_NUMPAD5;
	case vk::numpad6: return VK_NUMPAD6;
	case vk::numpad7: return VK_NUMPAD7;
	case vk::numpad8: return VK_NUMPAD8;
	case vk::numpad9: return VK_NUMPAD9;
	case vk::multiply: return VK_MULTIPLY;
	case vk::add: return VK_ADD;
	case vk::separator: return VK_SEPARATOR;
	case vk::subtract: return VK_SUBTRACT;
	case vk::decimal: return VK_DECIMAL;
	case vk::divide: return VK_DIVIDE;
	case vk::F1: return VK_F1;
	case vk::F2: return VK_F2;
	case vk::F3: return VK_F3;
	case vk::F4: return VK_F4;
	case vk::F5: return VK_F5;
	case vk::F6: return VK_F6;
	case vk::F7: return VK_F7;
	case vk::F8: return VK_F8;
	case vk::F9: return VK_F9;
	case vk::F10: return VK_F10;
	case vk::F11: return VK_F11;
	case vk::F12: return VK_F12;
	case vk::F13: return VK_F13;
	case vk::F14: return VK_F14;
	case vk::F15: return VK_F15;
	case vk::F16: return VK_F16;
	case vk::F17: return VK_F17;
	case vk::F18: return VK_F18;
	case vk::F19: return VK_F19;
	case vk::F20: return VK_F20;
	case vk::F21: return VK_F21;
	case vk::F22: return VK_F22;
	case vk::F23: return VK_F23;
	case vk::F24: return VK_F24;
	case vk::numlock: return VK_NUMLOCK;
	case vk::scroll: return VK_SCROLL;
	case vk::browser_back: return VK_BROWSER_BACK;
	case vk::browser_forward: return VK_BROWSER_FORWARD;
	case vk::browser_refresh: return VK_BROWSER_REFRESH;
	case vk::browser_stop: return VK_BROWSER_STOP;
	case vk::browser_search: return VK_BROWSER_SEARCH;
	case vk::browser_favorites: return VK_BROWSER_FAVORITES;
	case vk::browser_home: return VK_BROWSER_HOME;
	case vk::volume_mute: return VK_VOLUME_MUTE;
	case vk::volume_down: return VK_VOLUME_DOWN;
	case vk::volume_up: return VK_VOLUME_UP;
	case vk::media_next_track: return VK_MEDIA_NEXT_TRACK;
	case vk::media_prev_track: return VK_MEDIA_PREV_TRACK;
	case vk::media_stop: return VK_MEDIA_STOP;
	case vk::media_play_pause: return VK_MEDIA_PLAY_PAUSE;
	default:
		break;
	}

	return 0;
}

uint8_t mods2win32(const modifiers& val)
{
	BYTE out = FNOINVERT;

	if (val & modifier::shift)
		out |= FSHIFT;
	if (val & modifier::ctrl)
		out |= FCONTROL;
	if (val & modifier::alt)
		out |= FALT;

	return out;
}