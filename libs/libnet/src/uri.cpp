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
#include <cctype>

static inline bool issafe(unsigned char c)
{
	return std::isalnum(c) || c == '-' || c == '-' || c == '.' || c == '_' || c == '~';
}

template <typename Pred>
static inline std::string urlencode(const std::string& raw, Pred&& safe)
{
	static char hexes [] = "0123456789ABCDEF";
	std::string out;
	out.reserve(raw.length() * 11 / 10);

	for (unsigned char c : raw) {
		if (safe(c)) {
			out += c;
			continue;
		}
		out += '%';
		out += hexes[(c >> 4) & 0xF];
		out += hexes[(c) & 0xF];
	}
	return out;
}

static std::string urlencode(const std::string& raw)
{
	return urlencode(raw, issafe);
}

static inline bool auth_issafe(unsigned char c)
{
	return std::isalnum(c) || c == '-' || c == '-' || c == '.' || c == '_' || c == '~' || c == ':' || c == '[' || c == ']';
}

static std::string auth_urlencode(const std::string& raw)
{
	return urlencode(raw, auth_issafe);
}

static inline char hex(char c)
{
	switch (c) {
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		return c - '0';
	case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
		return c - 'a' + 10;
	case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
		return c - 'A' + 10;
	}
	return 0;
}

static std::string urldecode(const char* in, size_t in_len)
{
	std::string out;
	out.reserve(in_len);

	for (size_t i = 0; i < in_len; ++i) {
		// go inside only, if there is enough space
		if (in[i] == '%' && (i < in_len - 2) &&
			isxdigit(in[i + 1]) && isxdigit(in[i + 2])) {
			unsigned char c = (hex(in[i + 1]) << 4) | hex(in[i + 2]);
			out += c;
			i += 2;
			continue;
		}
		out += in[i];
	}
	return out;
}

static std::string urldecode(const std::string& in)
{
	return urldecode(in.c_str(), in.length());
}


Uri::Authority Uri::Authority::fromString(const std::string& authority)
{
	auto pos = authority.find('@');
	auto host = pos == std::string::npos ? 0 : pos + 1;

	auto colon = authority.find_last_of(':');
	if (authority.length() > host && authority[host] == '[') {
		// IPv6/IPVFuture...
		auto end = authority.find(']', host);
		if (end == std::string::npos)
			return { };

		if (authority.length() > end && authority[end + 1] != ':' && authority[end + 1] != 0)
			return { };

		colon = end + 1;
		if (colon >= authority.length())
			colon = std::string::npos;
	}
	auto host_count = colon == std::string::npos ? std::string::npos : colon - host;

	Authority out { };

	if (host)
		out.userInfo = urldecode(authority.substr(0, pos));

	out.host = urldecode(authority.substr(0, host_count));
	if (colon != std::string::npos)
		out.port = urldecode(authority.substr(colon + 1));

	return out;
}

std::string Uri::Authority::toString() const
{
	auto auserInfo = auth_urlencode(userInfo);
	auto ahost = auth_urlencode(host);
	auto aport = auth_urlencode(port);

	if (aport.empty() && auserInfo.empty())
		return ahost;

	size_t length = ahost.length();
	if (!auserInfo.empty())
		length += auserInfo.length() + 1;
	if (!aport.empty())
		length += aport.length() + 1;

	std::string out;
	out.reserve(length);

	if (!auserInfo.empty()) {
		out.append(auserInfo);
		out.push_back('@');
	}

	out.append(ahost);

	if (!aport.empty()) {
		out.push_back(':');
		out.append(aport);
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

namespace std {
	string tolower(string s)
	{
		for (auto& c : s)
			c = (char)tolower((uint8_t)c);

		return s;
	}
}

int default_port(const std::string& scheme)
{
#define KNOWN(proto, port) if (scheme == #proto) return port;

	KNOWN(http, 80); // ~100% of use cases...
	KNOWN(https, 443);

	KNOWN(ftp, 21);
	KNOWN(ssh, 22);
	KNOWN(telnet, 23);

#undef KNOWN
	return -1;
}

std::vector<std::string> path_split(const std::string& path)
{
	auto length = 1;
	for (auto c : path) {
		if (c == '/')
			++length;
	}

	std::vector<std::string> out;
	out.reserve(length);

	auto slash = path.find('/');
	decltype(slash) prev = 0;
	while (slash != std::string::npos) {
		out.emplace_back(path.substr(prev, slash - prev));
		prev = slash + 1;
		slash = path.find('/', prev);
	}
	out.emplace_back(path.substr(prev));

	return out;
}

std::string path_join(const std::vector<std::string>& chunks)
{
	if (chunks.empty())
		return { };

	auto length = chunks.size() - 1;
	for (auto& ch : chunks)
		length += ch.length();

	std::string out;
	out.reserve(length);

	bool first = true;
	for (auto& ch : chunks) {
		if (first) first = false;
		else out.push_back('/');

		out.append(ch);
	}

	return out;
}

Uri Uri::canonical(const Uri& uri, const Uri& base)
{
	if (uri.absolute())
		return normal(uri);

	// base-schema://base-auth/base-path?uri-query#uri-frag
	auto temp = base;
	temp.fragment(uri.fragment());
	temp.query(uri.query());

	auto path = uri.path();
	if (!path.empty() && path[0] == '/')
		return temp.path(path), normal(std::move(temp));

	auto bpath = base.path();
	if (!bpath.empty())
		return temp.path(base.path() + "/" + path), normal(std::move(temp));
	return temp.path(path), normal(std::move(temp));
}

Uri Uri::normal(Uri tmp)
{
	if (tmp.absolute() && tmp.hierarchical()) {
		auto scheme = std::tolower(tmp.scheme());
		tmp.scheme(scheme);

		auto auth = Authority::fromString(tmp.authority());
		if (auth.host.empty()) // decoding failed
			return { };

		// HOST: =======================================================================================
		auth.host = std::tolower(auth.host);

		// is IPv4, IPv6 or reg-name?
		for (auto c : auth.host) {
			if (!isalnum((uint8_t)c) && c != '-' && c != '.' && c != '[' && c != ']' && c != ':')
				return { };
		}

		// PORT: =======================================================================================
		// empty or digits
		for (auto c : auth.port) {
			if (!isdigit((uint8_t)c))
				return { };
		}

		// if default for the scheme, remove
		if (!auth.port.empty()) {
			auto port = atoi(auth.port.c_str());
			auto def = default_port(scheme);
			if (port == def)
				auth.port.clear();
		}

		tmp.authority(auth.toString());
	}
	// PATH: =======================================================================================
	if (tmp.hierarchical()) {

		auto path = path_split(tmp.path());
		for (auto& part : path)
			part = urlencode(urldecode(part));

		bool absolute = (path.size() > 1) && path.front().empty();

		// URL ended with slash; should still end with slash
		// Also, URL path ended with either xxxx/. or xxxxx/..
		// -> after resolving the result should be a "dir"
		bool empty_at_end = (path.size() > 1) &&
			(path.back().empty() || path.back() == "." || path.back() == "..");
		decltype(path) canon;
		canon.reserve(path.size());

		decltype(path) overshots;
		for (auto& p : path) {
			if (p.empty() || p == ".")
				continue;

			if (p == "..") {
				if (canon.empty())
					overshots.push_back(std::move(p));
				else
					canon.pop_back();
				continue;
			}

			canon.push_back(std::move(p));
		}
		if (empty_at_end)
			canon.emplace_back();
		if (absolute)
			tmp.path("/" + path_join(canon));
		else {
			canon.insert(canon.begin(), overshots.begin(), overshots.end());
			tmp.path(path_join(canon));
		}
	}

	return tmp;
}