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

#ifndef __JIRA_SERVER_HPP__
#define __JIRA_SERVER_HPP__

#include <jira/jira.hpp>
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace json
{
	struct value;
}

namespace net { namespace http { namespace client { struct XmlHttpRequest; }}}

namespace jira
{
	namespace secure {
		bool crypt(const std::vector<uint8_t>& plain, std::vector<uint8_t>& secret);
		bool decrypt(const std::vector<uint8_t>& secret, std::vector<uint8_t>& plain);
	}

	struct report {
		uint64_t startAt = 0;
		uint64_t total = 0;
		model schema;
		std::vector<record> data;
	};

	class search_def {
		std::string m_jql;
		std::vector<std::string> m_columns;
	public:
		search_def() = default;
		search_def(const std::string& jql, const std::string& columnsDescr);
		search_def(const std::string& jql, const std::vector<std::string>& columns);
		const std::string& jql() const { return m_jql; }
		const std::vector<std::string>& columns() const { return m_columns; }
		std::string columnsDescr() const;

		static const search_def standard;
	};

	class server {
		std::string m_name;
		std::string m_login;
		std::vector<uint8_t> m_password;
		std::string m_url;

		search_def m_view;

		std::string passwd() const;
	public:
		enum from_storage { stored };

		server() = default;
		server(const std::string& name, const std::string& login, const std::vector<uint8_t>& password, const std::string& url, const search_def& view, from_storage);
		server(const std::string& name, const std::string& login, const std::string& password, const std::string& url, const search_def& view);
		const std::vector<uint8_t>& password() const { return m_password; }
		const std::string& name() const { return m_name; }
		const std::string& login() const { return m_login; }
		const std::string& url() const { return m_url; }
		const search_def& view() const { return m_view; }

		void get(const std::string& uri, const std::function<void(net::http::client::XmlHttpRequest*)>& onDone);
		void loadJSON(const std::string& uri, const std::function<void (int, const json::value&)>& response);
		void search(const search_def& def, const std::function<void(int, report&&)>& response);
		void search(const std::function<void(int, report&&)>& response) { search(m_view, response); }
	};
}

#endif // __JIRA_SERVER_HPP__
