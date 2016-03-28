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
#include "json_settings.hpp"
#include "net/filesystem.hpp"

namespace settings {
	namespace {
		const char binary_prefix[] = "bin:";
	}

	fs::path configDir();

	std::shared_ptr<Section::Impl> Section::createJson(const std::string& organization, const std::string& application, const std::string& version)
	{
		fs::path path = configDir() / organization;
		if (!application.empty()) {
			path /= application;
			if (!version.empty()) {
				path /= version;
				path /= application;
			}
		}
		path += ".conf";

		return std::make_shared<JsonImpl>(path);
	}

	JsonManager::JsonManager(const fs::path& path)
		: m_path(path)
	{
	}

	struct FileCloser {
		void operator()(FILE* f)
		{
			fclose(f);
		}
	};

	std::unique_ptr<FILE, FileCloser> open_ptr(const fs::path& path, const char* mode)
	{
		return std::unique_ptr<FILE, FileCloser>(fopen(path.string().c_str(), mode));
	}

	static std::string content_of(const fs::path& path)
	{
		auto f = open_ptr(path, "r");
		if (!f)
			return { };

		fseek(f.get(), 0, SEEK_END);
		auto length = ftell(f.get());
		fseek(f.get(), 0, SEEK_SET);

		if (length < 1)
			return { };

		auto data = std::make_unique<char[]>((size_t)length);
		fread(data.get(), 1, (size_t)length, f.get());

		return data.get();
	}

	json::map JsonManager::getRoot()
	{
		if (m_root.is<json::map>())
			return m_root.as<json::map>();

		m_root = json::from_string(content_of(m_path));
		if (!m_root.is<json::map>())
			m_root = json::map();
		return m_root.as<json::map>();
	}

	void JsonManager::update()
	{
		fs::error_code ec;
		fs::create_directories(m_path.parent_path(), ec);
		if (ec)
			return;

		auto f = open_ptr(m_path, "w");
		if (!f)
			return;

		auto content = m_root.to_string(json::value::options::indented());
		fwrite(content.c_str(), 1, content.length(), f.get());
	}

	JsonImpl::JsonImpl(const fs::path& path)
		: m_mgr(std::make_shared<JsonManager>(path))
	{
		m_section = m_mgr->getRoot();
	}

	JsonImpl::JsonImpl(std::shared_ptr<JsonManager> mgr, const json::map& section)
		: m_mgr(std::move(mgr))
		, m_section(std::move(section))
	{
	}


	JsonImpl::~JsonImpl()
	{
	}

	std::shared_ptr<Section::Impl> JsonImpl::group(const std::string& name) const
	{
		auto it = m_section.find(name);
		if (it == m_section.end() || !it->second.is<json::MAP>())
			return std::make_shared<JsonImpl>(m_mgr);

		return std::make_shared<JsonImpl>(m_mgr, it->second.as<json::MAP>());
	}

	type JsonImpl::getType(const std::string& key) const
	{
		auto it = m_section.find(key);
		if (it == m_section.end())
			return None;

		auto type = it->second.get_type();
		switch (type) {
		case json::STRING:
			if (!it->second.as<std::string>().compare(0, sizeof(binary_prefix) - 1, binary_prefix))
				return Binary;
			return String;
		case json::VECTOR: return Binary;
		case json::INTEGER: return UInt64;
		};

		return None;
	}

	bool JsonImpl::hasGroup(const std::string& name) const
	{
		auto it = m_section.find(name);
		if (it == m_section.end())
			return false;

		return it->second.is<json::MAP>();
	}

	std::string JsonImpl::getString(const std::string& key) const
	{
		auto it = m_section.find(key);
		if (it == m_section.end())
			return { };

		auto& v = it->second;
		if (!v.is<json::STRING>() ||
			!v.as<std::string>().compare(0, sizeof(binary_prefix) - 1, binary_prefix))
			return { };
		return v.as<std::string>();
	}

	uint32_t JsonImpl::getUInt32(const std::string& key) const
	{
		auto it = m_section.find(key);
		if (it == m_section.end())
			return { };

		auto& v = it->second;
		if (!v.is<json::INTEGER>())
			return { };
		return (uint32_t)v.as<int32_t>();
	}

	uint64_t JsonImpl::getUInt64(const std::string& key) const
	{
		auto it = m_section.find(key);
		if (it == m_section.end())
			return { };

		auto& v = it->second;
		if (!v.is<json::INTEGER>())
			return { };
		return (uint64_t)v.as<int64_t>();
	}

	static uint8_t hex_val(char c)
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

	std::vector<uint8_t> JsonImpl::getBinary(const std::string& key) const
	{
		auto it = m_section.find(key);
		if (it == m_section.end())
			return { };

		auto& v = it->second;
		if (!v.is<json::STRING>() ||
			v.as<std::string>().compare(0, sizeof(binary_prefix) - 1, binary_prefix))
			return { };

		auto& s = v.as<std::string>();
		auto data = s.c_str() + sizeof(binary_prefix);
		auto count = s.length() > sizeof(binary_prefix) ? (s.length() - sizeof(binary_prefix)) / 2: 0;

		std::vector<uint8_t> out;
		out.reserve(count);
		for (size_t i = 0; i < count; ++i, data += 2) {
			uint8_t c = (hex_val(data[0]) << 16) | hex_val(data[1]);
			out.push_back(c);
		}
		return out;
	}

	void JsonImpl::setString(const std::string& key, const std::string& value)
	{
		auto it = m_section.find(key);
		if (it != m_section.end() && it->second.is<json::MAP>())
			return;

		m_section.add(key, value);
		m_mgr->update();
	}

	void JsonImpl::setUInt32(const std::string& key, uint32_t value)
	{
		auto it = m_section.find(key);
		if (it != m_section.end() && it->second.is<json::MAP>())
			return;

		m_section.add(key, (int32_t)value);
		m_mgr->update();
	}

	void JsonImpl::setUInt64(const std::string& key, uint64_t value)
	{
		auto it = m_section.find(key);
		if (it != m_section.end() && it->second.is<json::MAP>())
			return;

		m_section.add(key, (int64_t)value);
		m_mgr->update();
	}

	void JsonImpl::setBinary(const std::string& key, const std::vector<uint8_t>& value)
	{
		setBinary(key, value.data(), value.size());
	}

	void JsonImpl::setBinary(const std::string& key, const void* value, size_t size)
	{
		auto it = m_section.find(key);
		if (it != m_section.end() && it->second.is<json::MAP>())
			return;

		std::string data = binary_prefix;
		data.reserve(sizeof(binary_prefix) + 2 * size);

		static constexpr char alphabet[] = "0123456789ABCDEF";
		auto ptr = (const char*)value;
		for (size_t i = 0; i < size; ++i, ++ptr) {
			data.push_back(alphabet[(*ptr >> 4) & 0xF]);
			data.push_back(alphabet[*ptr & 0xF]);
		}

		m_section.add(key, data);
		m_mgr->update();
	}

	void JsonImpl::unset(const std::string& key)
	{
		auto it = m_section.find(key);
		if (it == m_section.end() || it->second.is<json::MAP>())
			return;

		m_section.erase(it);
		m_mgr->update();
	}

	void JsonImpl::unsetGroup(const std::string& key)
	{
		auto it = m_section.find(key);
		if (it == m_section.end() || !it->second.is<json::MAP>())
			return;

		m_section.erase(it);
		m_mgr->update();
	}
}
