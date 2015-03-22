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
#include <net/uri.hpp>
#include <net/xhr.hpp>
#include <string>

using namespace std::literals;

namespace jira
{
	search_def const search_def::standard{ "assignee=currentUser() and resolution=Unresolved"s, {
		"status"s,
		"assignee"s,
		"key"s,
		"priority"s,
		"summary"s,
		"resolution"s
	} };

	namespace {
		std::vector<std::string> split(const std::string& str, const std::string& sep)
		{
			std::vector<std::string> out;
			auto pos = str.find(sep, 0);
			decltype(pos) prev = 0;
			while (pos != std::string::npos) {
				out.push_back(str.substr(prev, pos - prev));
				prev = pos + sep.length();
				pos = str.find(sep, prev);
			}

			out.push_back(str.substr(prev));
			return out;
		}

		std::string join(const std::vector<std::string>& list, const std::string& sep)
		{
			if (list.empty())
				return{};

			size_t len = sep.length() * (list.size() - 1);

			for (auto& item : list)
				len += item.length();

			std::string out;
			out.reserve(len + 1);
			bool first = true;
			for (auto& item : list) {
				if (first) first = false;
				else out += sep;
				out += item;
			}
			return out;
		}
	}

	search_def::search_def(const std::string& jql, const std::string& columnsDescr)
		: m_jql(jql)
		, m_columns(split(columnsDescr, ","))
	{
		if (m_columns.size() == 1 && m_columns[0].empty())
			m_columns.clear();
	}

	search_def::search_def(const std::string& jql, const std::vector<std::string>& columns)
		: m_jql(jql)
		, m_columns(columns)
	{
	}

	std::string search_def::columnsDescr() const
	{
		return join(m_columns, ",");
	}

	server::server(const std::string& name, const std::string& login, const std::vector<uint8_t>& password, const std::string& url, const search_def& view, from_storage)
		: m_name(name)
		, m_login(login)
		, m_password(password)
		, m_url(url)
		, m_view(view)
	{
	}

	server::server(const std::string& name, const std::string& login, const std::string& password, const std::string& url, const search_def& view)
		: m_name(name)
		, m_login(login)
		, m_password()
		, m_url(url)
		, m_view(view)
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

	static char alphabet(size_t id)
	{
		static char alph [] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
		return alph[id];
	}

	// the output should be at least (len * 8 + 5) DIV 6 long
	void base64_encode(const void* data, size_t len, char* output)
	{
		const unsigned char* p = (const unsigned char*) data;
		size_t pos = 0;
		unsigned int bits = 0;
		unsigned int accu = 0;
		for (size_t i = 0; i < len; ++i) {
			accu = (accu << 8) | (p[i] & 0xFF);
			bits += 8;
			while (bits >= 6) {
				bits -= 6;
				output[pos++] = alphabet((accu >> bits) & 0x3F);
			}
		}
		if (bits > 0) {
			accu <<= 6 - bits;
			output[pos++] = alphabet(accu & 0x3F);
		}
	}

	void server::get(const std::string & uri, const std::function<void(net::http::client::XmlHttpRequest*)>& onDone)
	{
		using namespace net::http::client;

		auto xhr = create();
		xhr->setDebug();
		xhr->onreadystatechange([xhr, onDone](XmlHttpRequest* req) {
			if (req->getReadyState() != XmlHttpRequest::DONE)
				return;

			onDone(req);
			req->onreadystatechange(XmlHttpRequest::ONREADYSTATECHANGE{}); // clean up xhr 
		});

		xhr->open(HTTP_GET, Uri::canonical(uri, url()).string(), false);

		std::string auth;

		{
			auto plain = login() + ":" + passwd();
			auth.resize((plain.length() * 8 + 5) / 6);
			base64_encode(plain.data(), plain.length(), &auth[0]);
			plain.clear();
		}

		xhr->setRequestHeader("Authorization", "Basic " + auth);
		xhr->send();
	}

	void server::loadJSON(const std::string& uri, const std::function<void(int, const json::value&)>& response)
	{
		using namespace net::http::client;
		get(uri, [response](XmlHttpRequest* req) {
			if (req->getStatus() / 100 == 2) {
				auto text = req->getResponseText();
				auto length = req->getResponseTextLength();
				response(req->getStatus(), json::from_string(text, length));
			} else {
				response(req->getStatus(), json::value{});
			}
		});
	}

	void server::search(const search_def& def, const std::function<void(int, const report&)>& response)
	{
		if (url().empty()) {
			response(404, report{});
			return;
		}

		auto& jql = def.jql().empty() ? search_def::standard.jql() : def.jql();
		auto& columns = def.columns().empty() ? search_def::standard.columns() : def.columns();

		Uri uri{ "rest/api/2/search" };
		uri.query(Uri::QueryBuilder{}.add("jql", jql).add("fields",
			m_view.columns().empty() ? search_def::standard.columnsDescr() : m_view.columnsDescr()
			).string());

		auto base = url();
		loadJSON(uri.string(), [response, columns, base](int status, const json::value& data) {

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

