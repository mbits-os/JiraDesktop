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
#include "win32_settings.hpp"
#include <net/utf8.hpp>

namespace settings {
	std::shared_ptr<Section::Impl> Section::create(const std::string& organization, const std::string& application, const std::string& version)
	{
		std::string keyname = "Software\\" + organization;
		if (!application.empty()) {
			keyname += "\\" + application;
			if (!version.empty()) {
				keyname += "\\" + version;
			}
		}

		HKEY key = nullptr;
		auto ret = RegCreateKeyEx(HKEY_CURRENT_USER, utf::widen(keyname).c_str(), 0, nullptr, 0, KEY_ALL_ACCESS, nullptr, &key, nullptr);

		return std::make_shared<win32::Win32Impl>(key);
	}
}
namespace settings { namespace win32 {
	Win32Impl::Win32Impl(HKEY key) : m_key(key) {}
	Win32Impl::~Win32Impl()
	{
		if (m_key)
			RegCloseKey(m_key);
	}

	std::shared_ptr<Section::Impl> Win32Impl::group(const std::string& name) const
	{
		HKEY key = nullptr;
		auto ret = RegCreateKeyEx(m_key, utf::widen(name).c_str(), 0, nullptr, 0, KEY_ALL_ACCESS, nullptr, &key, nullptr);

		return std::make_shared<win32::Win32Impl>(key);
	}

	type Win32Impl::getType(const std::string& key) const
	{
		DWORD type = 0;
		if (RegQueryValueEx(m_key, utf::widen(key).c_str(), nullptr, &type, nullptr, nullptr))
			return None;
		switch (type) {
		case REG_SZ: return String;
		case REG_EXPAND_SZ: return String;
		case REG_BINARY: return Binary;
		case REG_DWORD: return UInt32;
		case REG_QWORD: return UInt64;
		};

		return None;
	}

	std::string Win32Impl::getString(const std::string& key) const
	{
		DWORD size = 0;
		auto wkey = utf::widen(key);

		if (RegQueryValueEx(m_key, wkey.c_str(), nullptr, nullptr, nullptr, &size))
			return{};

		if (size % sizeof(wchar_t))
			size += sizeof(wchar_t);
		size /= sizeof(wchar_t);

		std::wstring ws;
		ws.resize(size);
		size *= sizeof(wchar_t);

		if (RegQueryValueEx(m_key, wkey.c_str(), nullptr, nullptr, (LPBYTE)&ws[0], &size))
			return{};

		size = ws.length();
		if (size > 0) {
			while (size > 0 && ws[size - 1] == 0) --size;
			ws = ws.substr(0, size);
		}

		return utf::narrowed(ws);
	}

	uint32_t Win32Impl::getUInt32(const std::string& key) const
	{
		uint32_t out = 0;
		DWORD size = sizeof(out);
		RegQueryValueEx(m_key, utf::widen(key).c_str(), nullptr, nullptr, (LPBYTE) &out, &size);
		return out;
	}

	uint64_t Win32Impl::getUInt64(const std::string& key) const
	{
		uint32_t out = 0;
		DWORD size = sizeof(out);
		RegQueryValueEx(m_key, utf::widen(key).c_str(), nullptr, nullptr, (LPBYTE) &out, &size);
		return out;
	}

	std::vector<uint8_t> Win32Impl::getBinary(const std::string& key) const
	{
		DWORD size = 0;
		if (RegQueryValueEx(m_key, utf::widen(key).c_str(), nullptr, nullptr, nullptr, &size))
			return{};

		std::vector<uint8_t> out;
		out.resize(size);
		if (RegQueryValueEx(m_key, utf::widen(key).c_str(), nullptr, nullptr, &out[0], &size))
			return{};

		return out;
	}

	void Win32Impl::setString(const std::string& key, const std::string& value)
	{
		auto wvalue = utf::widen(value);
		RegSetValueEx(m_key, utf::widen(key).c_str(), 0, REG_SZ, (LPBYTE) &wvalue[0], sizeof(wchar_t) * (wvalue.length() + 1));
	}

	void Win32Impl::setUInt32(const std::string& key, uint32_t value)
	{
		RegSetValueEx(m_key, utf::widen(key).c_str(), 0, REG_DWORD, (LPBYTE) &value, sizeof(value));
	}

	void Win32Impl::setUInt64(const std::string& key, uint64_t value)
	{
		RegSetValueEx(m_key, utf::widen(key).c_str(), 0, REG_QWORD, (LPBYTE) &value, sizeof(value));
	}

	void Win32Impl::setBinary(const std::string& key, const std::vector<uint8_t>& value)
	{
		setBinary(key, value.data(), value.size());
	}

	void Win32Impl::setBinary(const std::string& key, const void* value, size_t size)
	{
		RegSetValueEx(m_key, utf::widen(key).c_str(), 0, REG_BINARY, (LPBYTE) value, size);
	}

	void Win32Impl::unset(const std::string& key)
	{
		RegDeleteValue(m_key, utf::widen(key).c_str());
	}

	void Win32Impl::unsetGroup(const std::string& key)
	{
		RegDeleteKey(m_key, utf::widen(key).c_str());
	}
}}
