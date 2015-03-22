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

#include <memory>
#include <string>
#include <vector>

namespace settings {
	enum type {
		None,
		String,
		UInt32,
		UInt64,
		Binary
	};

	class Section {
	public:
		class Impl;

		Section(const std::string& organization, const std::string& application = {}, const std::string& version = {});

		Section() = delete;
		Section(const Section&) = delete;
		Section& operator=(const Section&) = delete;
		Section(Section&&) = default;
		Section& operator=(Section&&) = default;
		~Section();

		Section group(const std::string& name) const;

		type getType(const std::string& key) const;
		bool contains(const std::string& key) const;

		std::string getString(const std::string& key) const;
		uint32_t getUInt32(const std::string& key) const;
		uint64_t getUInt64(const std::string& key) const;
		std::vector<uint8_t> getBinary(const std::string& key) const;

		void setString(const std::string& key, const std::string& value);
		void setUInt32(const std::string& key, uint32_t value);
		void setUInt64(const std::string& key, uint64_t value);
		void setBinary(const std::string& key, const std::vector<uint8_t>& value);
		void setBinary(const std::string& key, const void* value, size_t size);
		template <typename T> void setBinary(const std::string& key, const T& value) { return setBinary(key, &value, sizeof(T)); }

		void unset(const std::string& key);
		void unsetGroup(const std::string& key);

	private:
		std::shared_ptr<Impl> m_impl;
		static std::shared_ptr<Impl> create(const std::string& organization, const std::string& application, const std::string& version);
		Section(const std::shared_ptr<Impl>& impl);
	};
}
