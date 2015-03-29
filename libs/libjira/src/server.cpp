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
#include <atomic>

#if USE_ODS
#include <windows.h>
#endif

using namespace std::literals;

template <typename F>
struct on_exit_t {
	F fn;
	on_exit_t(const F& fn) : fn(fn) {}
	~on_exit_t() { fn(); }
};

#define ON_EXIT(FN) auto fn ## __LINE__ = FN; on_exit_t<decltype(fn ## __LINE__)> on_exit_ ## __LINE__ {fn ## __LINE__ };

namespace jira
{
	search_def const search_def::standard{ "assignee=currentUser() and resolution is empty"s,
		{ "status"s, "assignee"s, "key"s, "priority"s, "summary"s, "resolution"s }
	};

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

		uint32_t nextToken()
		{
			static std::atomic<uint32_t> token{ 0xba5e0000ul };
			return ++token;
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

	class server::credentials_provider : public net::http::client::CredentialProvider {
		std::shared_ptr<server> m_parent;
	public:
		explicit credentials_provider(const std::shared_ptr<server>& parent) : m_parent(parent) {}
		std::string getUsername() { return m_parent->login(); }
		std::string getPassword() { return m_parent->passwd(); }
	};

	server::server(const std::string& name, const std::string& login, const std::vector<uint8_t>& password, const std::string& url, const search_def& view, from_storage)
		: m_name(name)
		, m_login(login)
		, m_password(password)
		, m_url(url)
		, m_view(view)
		, m_db(url)
		, m_id(nextToken())
	{
		if (Uri{ url }.relative())
			m_url = "http://" + url;
	}

	server::server(const std::string& name, const std::string& login, const std::string& password, const std::string& url, const search_def& view)
		: m_name(name)
		, m_login(login)
		, m_password()
		, m_url(url)
		, m_view(view)
		, m_db(url)
		, m_id(nextToken())
	{
		if (Uri{ url }.relative())
			m_url = "http://" + url;

		if (!secure::crypt({ password.begin(), password.end() }, m_password))
			throw std::bad_alloc();
	}

	void server::onListenerAdded(const std::shared_ptr<server_listener>& listener)
	{
		if (m_isLoadingView)
			listener->onRefreshStarted();
	}

	std::string server::passwd() const
	{
		std::vector<uint8_t> plain;
		if (!secure::decrypt(m_password, plain))
			throw std::bad_alloc();

		return{ plain.begin(), plain.end() };
	}

	void server::loadFields()
	{
		m_isLoadingFields = true;
		loadJSON("rest/api/2/field", [this](int /*status*/, const json::value& doc) {
			m_db.reset_defs();

			ON_EXIT([this] {
				m_isLoadingFields = false;
				if (m_requestRefresh)
					refresh();
			});

			if (!doc.is<json::vector>())
				return;

#if USE_ODS
			std::map<std::string, bool> failed;
#endif

			for (const auto& vfld : doc.as<json::vector>()) {
				if (!vfld.is<json::map>())
					continue;

				auto fld = vfld.as<json::map>();
				auto schema_it = fld.find("schema");
				if (schema_it == fld.end())
					continue;

				auto schema = schema_it->second.as<json::map>()["type"].as_string();

				bool is_array = schema == "array";
				bool custom = fld["custom"].as<bool>();

				if (is_array)
					schema = schema_it->second.as<json::map>()["items"].as_string();

				auto id = fld["id"].as_string();
				auto it = fld.find("name");
				auto display = it == fld.end() ? id : it->second.as_string();

				if (!custom) {
					auto system = schema_it->second.as<json::map>()["system"].as_string();
					static const char* known_types [] = {
						"date",
						"datetime",
						"component",
						"issuelinks",
						"progress",
						"securitylevel",
						"user",
						"version"
					};

					bool known = schema == system;
					if (!known) {
						for (auto name : known_types) {
							if (schema == name) {
								known = true;
								break;
							}
						}
					}

					if (!known)
						schema = system;
				}

				if (!m_db.field_def(id, is_array, schema, display)) {
#if USE_ODS
					if (is_array)
						OutputDebugString(utf::widen("COULD NOT ADD: " + id + "(array of " + schema + ")\n").c_str());
					else
						OutputDebugString(utf::widen("COULD NOT ADD: " + id + "(" + schema + ")\n").c_str());
					failed[schema] = true;
#endif
				}
			}

#if USE_ODS
			if (!failed.empty()) {
				OutputDebugString(L"\nUnrecognized types:\n");
				for (auto& pair : failed) {
					OutputDebugString(utf::widen("  " + pair.first + "\n").c_str());
				}
				OutputDebugString(L"\n");
			}
#endif
		}, ONPROGRESS{}, true);
	}

	void server::refresh()
	{
		if (m_isLoadingView && !m_requestRefresh)
			return;

		if (!m_isLoadingView)
			emit([&](server_listener* listener) { listener->onRefreshStarted(); });

		m_requestRefresh = false;
		m_isLoadingView = true;

		if (m_isLoadingFields) {
			m_requestRefresh = true;
			return;
		}

		auto thiz = shared_from_this();
		search([thiz](int /*status*/, jira::report&& report) {
			ON_EXIT([thiz] {
				thiz->emit([&](server_listener* listener) { listener->onRefreshFinished(); });
				thiz->m_isLoadingView = false;
			});

			thiz->m_dataset = std::make_shared<jira::report>(std::move(report));
		}, [thiz](net::http::client::XmlHttpRequest* /*req*/, bool calculable, uint64_t content, uint64_t loaded) {
			thiz->emit([&](server_listener* listener) { listener->onProgress(calculable, content, loaded); });
		});

	}

	void server::debugDump(std::ostream& o)
	{
		m_db.debug_dump(o);
	}

	void server::get(const std::string & uri, const std::function<void(net::http::client::XmlHttpRequest*)>& onDone, const ONPROGRESS& progress, bool async)
	{
		using namespace net::http::client;

		auto xhr = create();
		// xhr->setDebug();
		xhr->onreadystatechange([xhr, onDone](XmlHttpRequest* req) {
			if (req->getReadyState() != XmlHttpRequest::DONE)
				return;

			onDone(req);
			req->onreadystatechange(XmlHttpRequest::ONREADYSTATECHANGE{}); // clean up xhr 
			req->onprogress(XmlHttpRequest::ONPROGRESS{}); // clean up xhr 
		});

		if (progress)
			xhr->onprogress([xhr, progress](bool calculable, uint64_t content, uint64_t loaded) { progress(xhr.get(), calculable, content, loaded); });

		xhr->open(HTTP_GET, Uri::canonical(uri, url()).string(), async);
		xhr->setCredentials(std::make_shared<credentials_provider>(shared_from_this()));
		xhr->send();
	}

	void server::loadJSON(const std::string& uri, const std::function<void(int, const json::value&)>& response, const ONPROGRESS& progress, bool async)
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
		}, progress, async);
	}

	void server::search(const search_def& def, const std::function<void(int, report&&)>& response, const ONPROGRESS& progress, bool async)
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
		loadJSON(uri.string(), [this, response, columns, base](int status, const json::value& data) {

			if ((status / 100) != 2) {
				response(status, report{});
				return;
			}

			json::map info{ data };

			report dataset;
			dataset.startAt = info["startAt"].as_int();
			dataset.total = info["total"].as_int();
			dataset.schema = m_db.create_model(columns);

			json::vector issues{ info["issues"] };
			for (auto& v_issue : issues) {
				json::map issue{ v_issue };
				auto key = issue["key"].as_string();
				auto id = issue["id"].as_string();

				dataset.data.push_back(dataset.schema.visit(issue["fields"], key, id));
			}

			response(status, std::move(dataset));
		}, progress, async);
	}
};

