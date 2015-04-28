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
#include <gui/settings.hpp>
#include "settings_impl.hpp"

namespace settings {
	Section::Section(const std::shared_ptr<Impl>& impl)
		: m_impl{ impl }
	{
	}
	
	Section::Section(const std::string& organization, const std::string& application, const std::string& version)
		: m_impl{ create(organization, application, version) }
	{
	};

	Section::~Section()
	{
	}

	Section Section::group(const std::string& name) const
	{
		return m_impl->group(name);
	}

	type Section::getType(const std::string& key) const
	{
		return m_impl->getType(key);
	}

	bool Section::contains(const std::string& key) const
	{
		return m_impl->getType(key) != None;
	}

	bool Section::hasGroup(const std::string& name) const
	{
		return m_impl->hasGroup(name);
	}

	std::string Section::getString(const std::string& key) const
	{
		return m_impl->getString(key);
	}

	uint32_t Section::getUInt32(const std::string& key) const
	{
		return m_impl->getUInt32(key);
	}

	uint64_t Section::getUInt64(const std::string& key) const
	{
		return m_impl->getUInt64(key);
	}

	std::vector<uint8_t> Section::getBinary(const std::string& key) const
	{
		return m_impl->getBinary(key);
	}

	void Section::setString(const std::string& key, const std::string& value)
	{
		return m_impl->setString(key, value);
	}

	void Section::setUInt32(const std::string& key, uint32_t value)
	{
		return m_impl->setUInt32(key, value);
	}

	void Section::setUInt64(const std::string& key, uint64_t value)
	{
		return m_impl->setUInt64(key, value);
	}

	void Section::setBinary(const std::string& key, const std::vector<uint8_t>& value)
	{
		return m_impl->setBinary(key, value);
	}

	void Section::setBinary(const std::string& key, const void* value, size_t size)
	{
		return m_impl->setBinary(key, value, size);
	}

	void Section::unset(const std::string& key)
	{
		return m_impl->unset(key);
	}

	void Section::unsetGroup(const std::string& key)
	{
		return m_impl->unsetGroup(key);
	}
}
