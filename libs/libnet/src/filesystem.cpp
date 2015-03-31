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

#include "pch.h"

#include <net/filesystem.hpp>
#include <vector>
#include <cstdlib>

namespace filesystem
{
	path_iterator::path_iterator() : m_ptr(0), m_offset(0) {}

	path_iterator::path_iterator(const path& val, size_t offset) : m_ptr(&val), m_offset(offset) { get_value(); }

	path_iterator::path_iterator(const path_iterator&) = default;
	path_iterator& path_iterator::operator=(const path_iterator&) = default;
	path_iterator::path_iterator(path_iterator&& oth)
		: m_ptr(oth.m_ptr)
		, m_value(std::move(oth.m_value))
		, m_offset(oth.m_offset)
		, m_needs_value(oth.m_needs_value)
	{
	}
	path_iterator& path_iterator::operator=(const path_iterator&& oth)
	{
		m_ptr = oth.m_ptr;
		m_value = std::move(oth.m_value);
		m_offset = oth.m_offset;
		m_needs_value = oth.m_needs_value;

		return *this;
	}

	path_iterator::reference path_iterator::operator*() const
	{
		if (m_needs_value)
			((path_iterator*)this)->get_value();
		return m_value;
	}

	path_iterator::pointer path_iterator::operator->() const
	{
		return (std::pointer_traits<pointer>::pointer_to(**this));
	}


	path_iterator& path_iterator::operator++()
	{
		size_t root_name_size = m_ptr->root_name_end();
		size_t path_size = m_ptr->m_path.size();
		const char* data = m_ptr->m_path.data();

		if (m_offset < root_name_size)
			m_offset = root_name_size; // move past drive

		else if (m_offset == root_name_size && root_name_size < path_size && data[root_name_size] == slash::value)
		{
			// move past root "/"
			for (++m_offset; m_offset < path_size; ++m_offset)
			{
				if (data[m_offset] != slash::value)
					break;
			}
		}
		else
		{
			// move past slashes followed by a name

			for (; m_offset < path_size; ++m_offset)
			{
				if (data[m_offset] != slash::value)
					break;
			}

			for (; m_offset < path_size; ++m_offset)
			{
				if (data[m_offset] == slash::value)
					break;
			}
		}

		m_needs_value = true;
		return (*this);
	}

	path_iterator path_iterator::operator++(int)
	{	// postincrement
		path_iterator _Tmp = *this;
		++*this;
		return (_Tmp);
	}

	path_iterator& path_iterator::operator--()
	{
		size_t offset_save = m_offset;
		size_t offset_prev = 0;

		m_offset = 0;
		do
		{
			offset_prev = m_offset;
			++*this;
		} while (m_offset < offset_save);
		m_offset = offset_prev;

		get_value();
		return *this;
	}

	path_iterator path_iterator::operator--(int)
	{	// postdecrement
		path_iterator _Tmp = *this;
		--*this;
		return (_Tmp);
	}

	bool path_iterator::operator==(const path_iterator& _Right) const
	{	// test for iterator equality
		return (m_ptr == _Right.m_ptr && m_offset == _Right.m_offset);
	}

	bool path_iterator::operator!=(const path_iterator& _Right) const
	{	// test for iterator inequality
		return (!(*this == _Right));
	}

	size_t path_iterator::__offset() const { return m_offset; }

	void path_iterator::get_value()
	{
		m_needs_value = false;

		size_t root_name_size = m_ptr->root_name_end();
		size_t path_size = m_ptr->m_path.size();

		m_value.clear();

		// nothing more to enumerate and/or end()
		if (path_size <= m_offset)
			return;

		if (m_offset < root_name_size)
		{
			m_value = m_ptr->m_path.substr(0, root_name_size); // get drive
			return;
		}

		const char* data = m_ptr->m_path.data();

		if (m_offset == root_name_size && root_name_size < path_size && data[root_name_size] == slash::value)
		{
			m_value = slash::value;	// get "/"
			return;
		}

		size_t start = m_offset;
		size_t next_slash = 0;

		for (; start < path_size; ++start)
		{
			if (data[start] != slash::value)
				break;
		}
		for (; start + next_slash < path_size; ++next_slash)
		{
			if (data[start + next_slash] == slash::value)
				break;
		}

		if (next_slash)
		{
			m_value = m_ptr->m_path.substr(start, next_slash);
			return;
		}

		if (start > m_offset) // we moved, but are right after the last slash
			m_value = dot::value;
	}

	path::path() = default;
	path::path(const path& p) = default;
	path::path(path&& p) : m_path(std::move(p.m_path)) {}
	path::path(const std::string& val) : m_path(val) { make_universal(m_path); }
	path::path(const char* val) : m_path(val) { make_universal(m_path); }
	path::~path() = default;

	path& path::operator=(const path& p) = default;
	path& path::operator=(path&& p)
	{
		m_path = std::move(p.m_path);
		return *this;
	}
	path& path::operator=(const std::string& val) { return *this = path(val); }
	path& path::operator=(const char* val) { return *this = path(val); }

	path& path::operator/=(const path& p) { return append(p.m_path); }
	path& path::operator/=(const std::string& val) { return append(val); }
	path& path::operator/=(const char* val) { return append(val); }
	path& path::append(const std::string& s) { return append(s.begin(), s.end()); }

#ifdef WIN32
	static std::string::const_iterator skip_drive(std::string::const_iterator from, std::string::const_iterator to)
	{
		auto save = from;
		while (from != to && *from != colon::value) ++from;
		if (from != to)
		{
			++from;
			if (from != to && (*from == slash::value) || (*from == backslash::value))
				return from;
		}

		return save;
	}
#endif

	path& path::operator+=(const path& val) { return *this += val.m_path; }

	path& path::append(std::string::const_iterator from, std::string::const_iterator to)
	{
#ifdef WIN32
		from = skip_drive(from, to);
#endif
		bool has_left_slash = !empty() && (*--m_path.end() == slash::value);
		bool has_right_slash = from != to && ((*from == directory_separator::value) || (*from == preferred_separator::value));

		if (has_left_slash && has_right_slash)
			++from;
		else if (!has_left_slash && !has_right_slash)
			m_path.push_back(slash::value);

		for (; from != to; ++from)
		{
			if (*from == preferred_separator::value)
				m_path.push_back(directory_separator::value);
			else
				m_path.push_back(*from);
		}

		return *this;
	}

	path& path::operator+=(const std::string& val)
	{
#ifdef WIN32
		std::string tmp{ val };
		make_universal(tmp);
		m_path += tmp;
#else
		m_path += val;
#endif
		return *this;
	}

	path& path::operator+=(const char* val)
	{
#ifdef WIN32
		std::string tmp{ val };
		make_universal(tmp);
		m_path += tmp;
#else
		m_path += val;
#endif
		return *this;
	}

	path& path::operator+=(char val)
	{
		if (val == preferred_separator::value)
			m_path.push_back(directory_separator::value);
		else
			m_path.push_back(val);
		return *this;
	}

#ifndef WIN32
	void path::make_universal(std::string&) {}
	size_t path::root_name_end() const { return 0; }
#endif
	size_t path::root_directory_end() const
	{
		auto name = root_name_end();
		if (name < m_path.size() && m_path[name] == slash::value)
			++name;
		return name;
	}

	void path::clear() { m_path.clear(); }
#ifndef WIN32
	path& path::make_preferred() { return *this; }
#endif

	path& path::remove_filename()
	{
		if (!empty() && begin() != --end())
		{
			// leaf to remove, back up over it
			size_t path_start = root_directory_end();
			size_t end = m_path.size();

			for (; path_start < end; --end)
			{
				if (m_path[end - 1] == slash::value)
					break;
			}
			for (; path_start < end; --end)
			{
				if (m_path[end - 1] != slash::value)
					break;
			}
			m_path.erase(end);
		}
		return *this;
	}

	path& path::replace_filename(const path& replacement) { return remove_filename() /= replacement; }
	path& path::replace_extension(const path& replacement)
	{
		if (replacement.empty() || replacement.m_path[0] == dot::value)
			return parent_path() /= (stem() + replacement);

		return parent_path() /= (stem() + "." + replacement);
	}

	void path::swap(path& rhs) noexcept { std::swap(m_path, rhs.m_path); }

	const std::string& path::string() const { return m_path; }

#ifndef WIN32
	std::string path::native() const { return m_path; }
#endif

	int path::compare(const path& p) const { return m_path.compare(p.string()); }
	int path::compare(const std::string& s) const { return compare(path(s)); }
	int path::compare(const char* s) const { return compare(path(s)); }

	path path::root_name() const
	{
		return m_path.substr(0, root_name_end());
	}

	path path::root_directory() const
	{
		auto pos = root_name_end();
		if (pos < m_path.size() && m_path[pos] == slash::value)
			return std::string(1, slash::value);

		return std::string();
	}

	path path::root_path() const
	{
		return m_path.substr(0, root_directory_end());
	}

	path path::relative_path() const
	{
		size_t pos = root_directory_end();

		while (pos < m_path.size() && m_path[pos] == slash::value)
			++pos;	// skip extra '/' after root

		return path(m_path.c_str() + pos, m_path.c_str() + m_path.size());
	}
	path path::parent_path() const
	{
		if (empty())
			return path();

		auto _end = --end();
		size_t off = _end.__offset();
		if (off && m_path[off - 1] == slash::value)
			--off;

		return m_path.substr(0, off);
	}

	path path::filename() const { return empty() ? std::string() : *--end(); }

	path path::stem() const
	{
		auto fname = filename().string();
		auto pos = fname.find(dot::value);
		return fname.substr(0, pos);
	}

	path path::extension() const
	{
		auto fname = filename().string();
		auto pos = fname.find(dot::value);
		return fname.substr(pos);
	}

	bool path::empty() const { return m_path.empty(); }
	bool path::has_root_name() const { return !root_name().empty(); }
	bool path::has_root_directory() const { return !root_directory().empty(); }
	bool path::has_root_path() const { return !root_path().empty(); }
	bool path::has_relative_path() const { return !relative_path().empty(); }
	bool path::has_parent_path() const { return !parent_path().empty(); }
	bool path::has_filename() const { return !filename().empty(); }
	bool path::has_stem() const { return !stem().empty(); }
	bool path::has_extension() const { return !extension().empty(); }
#ifdef WIN32
	bool path::is_absolute() const { return has_root_name() && has_root_directory(); }
#else
	bool path::is_absolute() const { return has_root_directory(); }
#endif
	bool path::is_relative() const { return !is_absolute(); }

	path::iterator path::begin() const
	{
		return (iterator(*this, (size_t)0));
	}

	path::iterator path::end() const
	{
		return (iterator(*this, m_path.size()));
	}

	path absolute(const path& p, const path& base)
	{
		if (p.is_absolute())
			return p;

		path a_base = base.is_absolute() ? base : absolute(base);

		if (p.has_root_name())
			return p.root_name() / a_base.root_directory() / a_base.relative_path() / p.relative_path();

		if (p.has_root_directory())
			return a_base.root_name() / p;

		return a_base / p;
	}

	path canonical(const path& p, const path& base)
	{
		path tmp = absolute(p, base);
		auto out = tmp.root_path();
		tmp = tmp.relative_path();

		std::vector<std::string> parts;
		bool last_dot = false;
		for (auto&& part : tmp)
		{
			last_dot = false;
			if (part == ".")
			{
				last_dot = true;
				continue;
			}

			if (part == "..")
			{
				if (!parts.empty())
					parts.pop_back();
				continue;
			}

			parts.push_back(part);
		}

		for (auto&& part : parts)
			out /= part;

		if (last_dot)
			out /= "";

		return out;
	}

	void remove(const path& p)
	{
		std::remove(p.native().c_str());
	}

	FILE* fopen(const path& file, char const* mode)
	{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996)
#endif
		return ::_wfopen(file.wnative().c_str(), utf::widen(mode).c_str());
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	}
}
