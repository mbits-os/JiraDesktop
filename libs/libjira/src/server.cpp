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

#include "jira/server.hpp"

namespace jira
{
	server::server(const std::string& login, const std::vector<uint8_t>& password, const std::string& url, from_storage)
		: m_login(login)
		, m_password(password)
		, m_url(url)
	{
	}

	server::server(const std::string& login, const std::string& password, const std::string& url)
		: m_login(login)
		, m_password()
		, m_url(url)
	{
		if (!secure::crypt({ password.begin(), password.end() }, m_password))
			throw std::bad_alloc();
	}

	std::string server::passwd() const
	{
		std::vector<uint8_t> plain;
		if (!secure::decrypt(m_password, plain))
			throw std::bad_alloc();

		return{ plain.begin(), plain.end() };
	}

	void server::loadJSON(const std::string& uri, const std::function<void(int, const json::value&)>& response)
	{
		response(404, json::value{});
	}

	void server::search(const std::string& jql, const std::vector<std::string>& columns,
		const std::function<void(int, const report&)>& response)
	{
		std::string uri = url();
		if (uri.empty()) {
			response(404, report{});
			return;
		}

		if (uri[uri.length() - 1] != '/')
			uri.push_back('/');
		uri += "rest/api/2/search?jql=";
		uri += jql;

		auto base = url();
		loadJSON(uri, [response, columns, base](int status, const json::value& data) {

			if ((status / 100) != 2) {
				response(status, report{});
				return;
			}

			jira::db db{ base };
			auto model = db.create_model(columns);

			report dataset;
			json::map info{ data };
			dataset.startAt = info["startAt"].as_int();
			dataset.total = info["total"].as_int();
			json::vector issues{ info["issues"] };

			for (auto& v_issue : issues) {
				json::map issue{ v_issue };
				json::map fields{ issue["fields"] };
				auto key = issue["key"].as_string();
				auto id = issue["id"].as_string();

				dataset.data.push_back(model.visit(fields, key, id));
			}

			response(status, dataset);
		});
	}
};

