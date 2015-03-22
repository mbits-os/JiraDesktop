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

	void server::loadJSON(const std::string& uri, const std::function<void(int, const json::value&)>& response)
	{
		auto xhr = net::http::client::create();
		using namespace net::http::client;
		xhr->onreadystatechange([xhr, response](XmlHttpRequest* req) {
			auto state = req->getReadyState();
			if (state != XmlHttpRequest::DONE)
				return;
			if (req->getStatus() / 100 == 2) {
				auto text = req->getResponseText();
				auto length = req->getResponseTextLength();
				response(req->getStatus(), json::from_string(text, length));
			} else {
				response(req->getStatus(), json::value{});
			}
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

	void server::search(const std::string& jql, const std::vector<std::string>& columns,
		const std::function<void(int, const report&)>& response)
	{
		if (url().empty()) {
			response(404, report{});
			return;
		}

		Uri uri{ "rest/api/2/search" };
		uri.query(Uri::QueryBuilder{}.add("jql", jql).string());

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

