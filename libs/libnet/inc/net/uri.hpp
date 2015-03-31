/*
 * Copyright (C) 2013 midnightBITS
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

#ifndef __URI_HPP__
#define __URI_HPP__

#include <stdint.h>
#include <net/filesystem.hpp>
#include <map>
#include <vector>

#ifdef LIBNET_EXPORTS
#define URI_LINK __declspec(dllexport)
#else
#define URI_LINK
#endif

class URI_LINK Uri
{
	std::string m_uri;

	static const std::string::size_type ncalc = (std::string::size_type)(-2);
	static const std::string::size_type npos = (std::string::size_type)(-1);

	mutable std::string::size_type m_schema = ncalc;
	mutable std::string::size_type m_path = ncalc;
	mutable std::string::size_type m_query = ncalc;
	mutable std::string::size_type m_part = ncalc;

	void ensure_schema() const;
	void ensure_path() const;
	void ensure_query() const;
	void ensure_fragment() const;
	void invalidate_fragment();
	void invalidate_query();
	void invalidate_path();
	void invalidate_schema();
public:
	Uri();
	Uri(const Uri&);
	Uri(Uri&&);
	Uri& operator=(const Uri&);
	Uri& operator=(Uri&&);
	Uri(const std::string& uri);

	bool hierarchical() const;
	bool opaque() const;
	bool relative() const;
	bool absolute() const;
	std::string scheme() const;
	std::string authority() const;
	std::string path() const;
	std::string query() const;
	std::string fragment() const;
	void path(const std::string& value);
	void query(const std::string& value);
	void fragment(const std::string& value);
	std::string string() const;

	static Uri make_base(const std::string& document);
	static Uri make_base(const Uri& document);
	static Uri canonical(const Uri& uri, const Uri& base);

	struct URI_LINK QueryBuilder {
		std::map<std::string, std::vector<std::string>> m_values;
	public:
		QueryBuilder& add(const std::string& name, const std::string& value);
		std::string string() const;
	};
};

#endif //__URI_HPP__
