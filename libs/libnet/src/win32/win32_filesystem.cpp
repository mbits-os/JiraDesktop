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

#include <pch.h>
#include <windows.h>
#include <net/filesystem.hpp>
#include <sys/stat.h>
#include <iterator>

namespace filesystem
{
	path::path(const std::wstring& val) : m_path(utf::narrowed(val)) { make_universal(m_path); }
	path::path(const wchar_t* val) : m_path(utf::narrowed(val)) { make_universal(m_path); }

	void path::make_universal(std::string& s)
	{
		for (char& c : s)
		{
			if (c == backslash::value)
				c = slash::value;
		}
	}

	size_t path::root_name_end() const
	{
		auto pos = m_path.find(colon::value);
		if (pos == std::string::npos)
			return 0;

		return pos + 1;
	}

	path& path::make_preferred()
	{
		for (char& c : m_path)
		{
			if (c == slash::value)
				c = backslash::value;
		}

		return *this;
	}

	std::wstring path::wstring() const { return utf::widen(m_path); }
	std::wstring path::wnative() const { return path(*this).make_preferred().wstring(); }
	std::string path::native() const { return path(*this).make_preferred().string(); }

	path current_path()
	{
		wchar_t buffer[2048];
		GetCurrentDirectory(sizeof(buffer), buffer);
		return buffer;
	}

	path app_directory()
	{
		wchar_t buffer[2048];
		GetModuleFileName(nullptr, buffer, sizeof(buffer));
		return path(buffer).parent_path();
	}

	status::status(const path& p)
	{
		auto native = p.wnative();
		DWORD atts = GetFileAttributes(native.c_str());
		if (atts == INVALID_FILE_ATTRIBUTES)
		{
			m_type = file_type::not_found;
			return;
		}

		if (atts & FILE_ATTRIBUTE_DIRECTORY)
			m_type = file_type::directory;
		else
			m_type = file_type::regular;

		struct _stat64i32 st;
		if (!_wstat(native.c_str(), &st))
		{
			m_file_size = st.st_size;
			m_mtime = st.st_mtime;
			m_ctime = st.st_ctime;
		}
		else
			m_type = file_type::not_found;
	}

	void directory_iterator::setup_iter(const path& p)
	{
		m_find.m_handle = FindFirstFileW((p / "*").wnative().c_str(), &m_find.m_data);
		if (m_find.m_handle != INVALID_HANDLE_VALUE)
			m_entry = directory_entry{ p / m_find.m_data.cFileName };
	}

	void directory_iterator::next_entry()
	{
		if (m_find.m_handle == INVALID_HANDLE_VALUE)
			return;

		if (!FindNextFile(m_find.m_handle, &m_find.m_data)) { // we are at end()
			m_find.close();
			m_entry = directory_entry{};
			return;
		}

		m_entry = directory_entry{ m_entry.path().parent_path() / m_find.m_data.cFileName };
	}

	void directory_iterator::shutdown_iter()
	{
		// noop
	}

	directory_iterator::directory_iterator(directory_iterator&& rhs)
		: m_entry(std::move(rhs.m_entry))
	{
		std::swap(m_find, rhs.m_find);
	}

	directory_iterator& directory_iterator::operator=(const directory_iterator& rhs)
	{
		if (m_find.m_handle == rhs.m_find.m_handle)
			return *this;

		m_find.close();
		if (rhs.m_find.m_handle != INVALID_HANDLE_VALUE)
			setup_iter(rhs.m_entry.path().parent_path());

		return *this;
	}

	directory_iterator& directory_iterator::operator=(directory_iterator&& rhs)
	{
		m_entry = std::move(rhs.m_entry);
		std::swap(m_find, rhs.m_find);
		return *this;
	}

}
