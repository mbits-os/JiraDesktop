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
#include <net/uri.hpp>

static std::string urlencode(const std::string& raw)
{
	static char hexes [] = "0123456789ABCDEF";
	std::string out;
	out.reserve(raw.length() * 11 / 10);

	for (unsigned char c : raw) {
		if (isalnum(c) || c == '-' || c == '-' || c == '.' || c == '_' || c == '~') {
			out += c;
			continue;
		}
		out += '%';
		out += hexes[(c >> 4) & 0xF];
		out += hexes[(c) & 0xF];
	}
	return out;
}

std::string Uri::QueryBuilder::string() const
{
	std::string out;
	bool first = true;
	for (auto& pair : m_values) {
		auto name = urlencode(pair.first) + "=";
		for (auto& value : pair.second) {
			if (first) {
				first = false;
				out += "?";
			}
			else out += "&";

			out += name + urlencode(value);
		}
	}
	return out;
}