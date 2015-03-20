/*
 * Copyright (C) 2014 midnightBITS
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

#ifndef __JIRA_JIRA_HPP__
#define __JIRA_JIRA_HPP__

#include <json/json.hpp>
#include <memory>
#include <sstream>

namespace jira
{
	struct type {
		type(const char* name) : m_name(name) {}
		virtual ~type() {}
		virtual const char* key() const { return m_name; }
		virtual std::string visit(const json::map& object, const std::string& key) const = 0;
	private:
		const char* m_name;
	};

	class key : public type {
	public:
		key(const char* k) : type(k) {}
		std::string visit(const json::map& /*object*/, const std::string& k) const override
		{
			return k;
		}
	};

	class string : public type {
	public:
		string(const char* key) : type(key) {}
		std::string visit(const json::map& object, const std::string& /*key*/) const override
		{
			auto it = object.find(key());
			if (it == object.end())
				return std::string();

			return it->second.as_string();
		}
	};

	class user : public type {
	public:
		user(const char* key) : type(key) {}
		std::string visit(const json::map& object, const std::string& /*key*/) const override
		{
			auto it = object.find(key());
			if (it == object.end() || !it->second.is<json::MAP>())
				return std::string();

			json::map data{ it->second };
			auto display = data["displayName"];
			if (!display.is<json::STRING>())
				return "?";

			return display.as_string();
		}
	};

	class icon : public type {
	public:
		icon(const char* key) : type(key) {}
		std::string visit(const json::map& object, const std::string& /*key*/) const override
		{
			auto it = object.find(key());
			if (it == object.end() || !it->second.is<json::MAP>())
				return std::string();

			json::map data{ it->second };
			auto display = data["name"];
			if (!display.is<json::STRING>())
				return "?";

			return display.as_string();
		}
	};

	class column {
		std::unique_ptr<type> m_visitor;
		const char* m_pre;
		const char* m_post;
	public:
		column(std::unique_ptr<type>&& visitor,
			const char* pre, const char* post)
			: m_visitor(std::move(visitor))
			, m_pre(pre)
			, m_post(post)
		{
		}

		std::string visit(const json::map& object, const std::string& key) const
		{
			if (!m_pre) {
				if (!m_post)
					return m_visitor->visit(object, key);
				return m_visitor->visit(object, key) + m_post;
			}
			if (!m_post)
				return m_pre + m_visitor->visit(object, key);
			return m_pre + m_visitor->visit(object, key) + m_post;
		}
	};

	class row {
		std::vector<jira::column> m_cols;
	public:
		template <typename T>
		row& column(const char* key)
		{
			m_cols.emplace_back(
				std::make_unique<T>(key), nullptr, nullptr
				);
			return *this;
		}
		template <typename T>
		row& column(const char* key, const char* pre, const char* post)
		{
			m_cols.emplace_back(
				std::make_unique<T>(key), pre, post
				);
			return *this;
		}

		std::string visit(const json::map& object, const std::string& key) const
		{
			std::ostringstream o;
			bool first = true;
			for (auto& col : m_cols) {
				if (first) first = false;
				else o << ' ';
				o << col.visit(object, key);
			}
			return o.str();
		}
	};
}

#endif // __JIRA_JIRA_HPP__
