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

#ifndef __FILESYSTEM_HPP__
#define __FILESYSTEM_HPP__

#include <string>
#include <net/utf8.hpp>
#ifdef WIN32
#	include <windows.h>
#endif

#ifdef LIBNET_EXPORTS
#define FS_LINK __declspec(dllexport)
struct __declspec(dllimport) _WIN32_FIND_DATAW;
#else
#define FS_LINK
#endif

namespace filesystem
{
	struct slash { static const char value = '/'; };
	struct dot { static const char value = '.'; };
	struct backslash { static const char value = '\\'; };
	struct colon { static const char value = ':'; };

	class path;

	class FS_LINK path_iterator
		: public std::iterator< std::bidirectional_iterator_tag, std::string, std::ptrdiff_t, std::string*, std::string&>
	{
	public:
		typedef const std::string *pointer;
		typedef const std::string& reference;

		path_iterator();
		path_iterator(const path& val, size_t offset);
		path_iterator(const path_iterator&);
		path_iterator& operator=(const path_iterator&);
		path_iterator(path_iterator&& oth);
		path_iterator& operator=(const path_iterator&& oth);
		reference operator*() const;
		pointer operator->() const;
		path_iterator& operator++();
		path_iterator operator++(int);
		path_iterator& operator--();
		path_iterator operator--(int);
		bool operator==(const path_iterator& _Right) const;
		bool operator!=(const path_iterator& _Right) const;
		size_t __offset() const;
	private:
		inline void get_value();

		const path *m_ptr;
		std::string m_value;
		size_t m_offset;
		bool m_needs_value;
	};

	class FS_LINK path
	{
		friend class path_iterator;
		std::string m_path;

#ifdef WIN32
		static void make_universal(std::string& s);
		size_t root_name_end() const;
#else
		static void make_universal(std::string&) {}
		size_t root_name_end() const { return 0; }
#endif
		size_t root_directory_end() const;

	public:
		using directory_separator = slash;
#ifdef WIN32
		using preferred_separator = backslash;
#else
		using preferred_separator = slash;
#endif

		path();
		path(const path& p);
		path(path&& p);
		path(const std::string& val);
		path(const char* val);
#ifdef WIN32
		path(const std::wstring& val);
		path(const wchar_t* val);
#endif
		template <typename It>
		path(It begin, It end) : m_path(begin, end) { make_universal(m_path); }
		~path();

		path& operator=(const path& p);
		path& operator=(path&& p);
		path& operator=(const std::string& val);
		path& operator=(const char* val);

		path& operator/=(const path& p);
		path& operator/=(const std::string& val);
		path& operator/=(const char* val);
		path& append(const std::string& s);
		path& append(std::string::const_iterator from, std::string::const_iterator to);


		path& operator+=(const path& val);
		path& operator+=(const std::string& val);
		path& operator+=(const char* val);
		path& operator+=(char val);


		void clear();
#ifdef WIN32
		path& make_preferred();
#else
		path& make_preferred() { return *this; }
#endif
		path& remove_filename();
		path& replace_filename(const path& replacement);
		path& replace_extension(const path& replacement = path());
		void swap(path& rhs) noexcept;

		const std::string& string() const;
#ifdef WIN32
		std::wstring wstring() const;
		std::wstring wnative() const;
#endif
		std::string native() const;

		int compare(const path& p) const;
		int compare(const std::string& s) const;
		int compare(const char* s) const;

		path root_name() const;
		path root_directory() const;
		path root_path() const;
		path relative_path() const;
		path parent_path() const;
		path filename() const;
		path stem() const;
		path extension() const;

		bool empty() const;
		bool has_root_name() const;
		bool has_root_directory() const;
		bool has_root_path() const;
		bool has_relative_path() const;
		bool has_parent_path() const;
		bool has_filename() const;
		bool has_stem() const;
		bool has_extension() const;
		bool is_absolute() const;
		bool is_relative() const;

		using iterator = path_iterator;
		using const_iterator = iterator;

		iterator begin() const;
		iterator end() const;
	};

	inline void swap(path& lhs, path& rhs)
	{
		lhs.swap(rhs);
	}

	inline bool operator< (const path& lhs, const path& rhs)
	{
		return lhs.compare(rhs) < 0;
	}

	inline bool operator<=(const path& lhs, const path& rhs)
	{
		return !(rhs < lhs);
	}

	inline bool operator>(const path& lhs, const path& rhs)
	{
		return rhs < lhs;
	}

	inline bool operator>=(const path& lhs, const path& rhs)
	{
		return !(lhs < rhs);
	}

	inline bool operator==(const path& lhs, const path& rhs)
	{
		return !(lhs < rhs) && !(rhs < lhs);
	}

	inline bool operator!=(const path& lhs, const path& rhs)
	{
		return !(lhs == rhs);
	}

	template <typename T>
	inline path operator/ (const path& lhs, const T& rhs)
	{
		return path(lhs) /= rhs;
	}

	template <typename T>
	inline path operator+ (const path& lhs, const T& rhs)
	{
		return path(lhs) += rhs;
	}

	template <class charT, class traits>
	inline std::basic_ostream<charT, traits>& operator<<(std::basic_ostream<charT, traits>& os, const path& p)
	{
		return os << p.string();
	}

	path FS_LINK current_path();
	inline void current_path(path& p) { p = current_path(); }

#ifdef WIN32
	path FS_LINK app_directory();
#endif

	path FS_LINK absolute(const path& p, const path& base = current_path());
	path FS_LINK canonical(const path& p, const path& base = current_path());

	enum class file_type
	{
		not_found = -1,
		none,
		regular,
		directory,
		symlink,
		block,
		character,
		fifo,
		socket,
		unknown
	};

	class FS_LINK status
	{
		file_type m_type = file_type::none;
		uintmax_t m_file_size = 0;
		time_t    m_mtime = 0;
		time_t    m_ctime = 0;
	public:
		status() {}
		status(const status&) = default;
		explicit status(const path&);
		status& operator=(const status&) = default;

		file_type type() const { return m_type; }
		bool exists() const
		{
			switch (type())
			{
			case file_type::none:
			case file_type::not_found:
				return false;
			default:
				break;
			}
			return true;
		}
		uintmax_t file_size() const { return m_file_size; }
		time_t mtime() const { return m_mtime; }
		time_t ctime() const { return m_ctime; }
		bool is_directory() const { return m_type == file_type::directory; }
		bool is_regular_file() const { return m_type == file_type::regular; }
	};

	inline bool exists(const path& p) { return status(p).exists(); }
	inline uintmax_t file_size(const path& p) { return status(p).file_size(); }
	inline bool is_directory(const path& p) { return status(p).is_directory(); }
	inline bool is_regular_file(const path& p) { return status(p).is_regular_file(); }

	FS_LINK void remove(const path& p);
	FS_LINK FILE* fopen(const path& file, char const* mode);

	class FS_LINK directory_entry
	{
	public:
		// constructors and destructor
		directory_entry() = default;
		directory_entry(const directory_entry&) = default;
		directory_entry(directory_entry&&) = default;

		explicit directory_entry(const filesystem::path& p) : m_path(p) {};
		~directory_entry() {}
		// modifiers
		directory_entry& operator=(const directory_entry&) = default;
		directory_entry& operator=(directory_entry&&) = default;
		void assign(const path& p) { m_path = p; }
		void replace_filename(const path& p) {
			m_path = m_path.parent_path() / p;
		}
		// observers
		const path& path() const { return m_path; }
		operator const filesystem::path&() const { return m_path; }
		filesystem::status status() const { return filesystem::status{ m_path }; }
		bool operator< (const directory_entry& rhs) const { return m_path < rhs.m_path; }
		bool operator==(const directory_entry& rhs) const { return m_path == rhs.m_path; }
		bool operator!=(const directory_entry& rhs) const { return m_path != rhs.m_path; }
		bool operator<=(const directory_entry& rhs) const { return m_path <= rhs.m_path; }
		bool operator> (const directory_entry& rhs) const { return m_path > rhs.m_path; }
		bool operator>=(const directory_entry& rhs) const { return m_path >= rhs.m_path; }
	private:
		filesystem::path m_path; // for exposition only
	};

	class FS_LINK directory_iterator
	{
		directory_entry m_entry;
		void setup_iter(const path& p); // should do the initial next_entry
		void next_entry();
		void shutdown_iter();

#ifdef WIN32
		struct FS_LINK dir_find {
			WIN32_FIND_DATAW m_data;
			HANDLE m_handle;
			dir_find() : m_handle(INVALID_HANDLE_VALUE) {}
			~dir_find() { close(); }

			void close() {
				if (m_handle != INVALID_HANDLE_VALUE)
					FindClose(m_handle);
				m_handle = INVALID_HANDLE_VALUE;
			}
		};
#else
#		error Define directory iteration...
#endif
		dir_find m_find;
	public:
		typedef directory_entry value_type;
		typedef ptrdiff_t difference_type;
		typedef const directory_entry* pointer;
		typedef const directory_entry& reference;
		typedef std::input_iterator_tag iterator_category;

		directory_iterator() = default;
		explicit directory_iterator(const path& p)
		{
			setup_iter(p);
		}
		directory_iterator(const directory_iterator& rhs)
		{
			if (rhs.m_entry != directory_entry{}) {
				setup_iter(rhs.m_entry.path().parent_path());
			}
		}
		directory_iterator(directory_iterator&& rhs);
		~directory_iterator() { shutdown_iter(); }
		directory_iterator& operator=(const directory_iterator& rhs);
		directory_iterator& operator=(directory_iterator&& rhs);
		const directory_entry& operator*() const { return m_entry; }
		const directory_entry* operator->() const { return &m_entry; }
		directory_iterator& operator++() {
			next_entry();
			return *this;
		}

		bool operator!=(const directory_iterator& rhs) const { return m_entry != rhs.m_entry; }
	};

	class FS_LINK dir_entries
	{
		path m_dir;
	public:
		explicit dir_entries(const path& dir) : m_dir(dir) {}
		directory_iterator begin() const { return directory_iterator(m_dir); }
		directory_iterator end() const { return directory_iterator(); }
	};

	inline dir_entries dir(const path& dir) { return dir_entries{ dir }; }
};

namespace std
{
	inline void swap(filesystem::path& lhs, filesystem::path& rhs)
	{
		lhs.swap(rhs);
	}
}

namespace fs = filesystem;

#endif // __FILESYSTEM_HPP__
