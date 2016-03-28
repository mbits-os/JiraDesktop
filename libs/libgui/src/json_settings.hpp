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

#include <gui/settings.hpp>
#include "settings_impl.hpp"
#include "net/filesystem.hpp"
#include <json/json.hpp>

namespace settings {
	class JsonManager {
		fs::path m_path;
		json::value m_root;
	public:
		JsonManager(const fs::path& path);
		json::map getRoot();
		void update();
	};

	class JsonImpl : public Section::Impl {
		std::shared_ptr<JsonManager> m_mgr;
		json::map m_section;
	public:
		JsonImpl(const fs::path& path);
		JsonImpl(std::shared_ptr<JsonManager> mgr, const json::map& section = { });
		~JsonImpl();

		std::shared_ptr<Section::Impl> group(const std::string& name) const override;
		type getType(const std::string& key) const override;
		bool hasGroup(const std::string& name) const override;

		std::string getString(const std::string& key) const override;
		uint32_t getUInt32(const std::string& key) const override;
		uint64_t getUInt64(const std::string& key) const override;
		std::vector<uint8_t> getBinary(const std::string& key) const override;

		void setString(const std::string& key, const std::string& value) override;
		void setUInt32(const std::string& key, uint32_t value) override;
		void setUInt64(const std::string& key, uint64_t value) override;
		void setBinary(const std::string& key, const std::vector<uint8_t>& value) override;
		void setBinary(const std::string& key, const void* value, size_t size) override;
		void unset(const std::string& key) override;
		void unsetGroup(const std::string& key) override;
	};
}
