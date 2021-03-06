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

#ifdef max
#undef max
#endif

using namespace std::literals;
using namespace std::chrono_literals;

template <typename F>
struct on_exit_t {
	F fn;
	on_exit_t(const F& fn) : fn(fn) {}
	~on_exit_t() { fn(); }
};

#define ON_EXIT(FN) auto fn ## __LINE__ = FN; on_exit_t<decltype(fn ## __LINE__)> on_exit_ ## __LINE__ {fn ## __LINE__ };

namespace jira
{
	search_def const search_def::standard{ "Assigned to Me"s, "assignee=currentUser() and resolution is empty"s,
		{ "status"s, "assignee"s, "key"s, "priority"s, "summary"s, "resolution"s },
		15min
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

	search_def::search_def() = default;
	search_def::search_def(const search_def&) = default;
	search_def::search_def(search_def&&) = default;
	search_def& search_def::operator=(const search_def&) = default;
	search_def& search_def::operator=(search_def&&) = default;

	search_def::search_def(const std::string& title, const std::string& jql, const std::string& columnsDescr, std::chrono::milliseconds timeout)
		: m_title(title)
		, m_jql(jql)
		, m_columns(split(columnsDescr, ","))
		, m_timeout((size_t)timeout.count())
		, m_id(nextToken())
	{
		if (m_columns.size() == 1 && m_columns[0].empty())
			m_columns.clear();
	}

	search_def::search_def(const std::string& title, const std::string& jql, const std::vector<std::string>& columns, std::chrono::milliseconds timeout)
		: m_title(title)
		, m_jql(jql)
		, m_columns(columns)
		, m_timeout((size_t)timeout.count())
		, m_id(nextToken())
	{
	}

	std::string search_def::columnsDescr() const
	{
		return join(m_columns, ",");
	}

	class server::credentials_provider : public net::http::client::CredentialProvider {
		std::shared_ptr<server> m_parent;
		gui::credential_ui_ptr m_auth_ui;

		class server_credentials : public gui::credential_ui::owner {
			std::shared_ptr<server> m_parent;
		public:
			server_credentials(const std::shared_ptr<server>& parent) : m_parent(parent) {}
			std::string key() const override { return "Server@" + std::to_string(m_parent->sessionId()); }
			std::string get_username() const override { return m_parent->login(); }
			std::string get_password() const override { return m_parent->passwd(); }
			void set_credentials(const std::string& login, const std::string& password) override
			{
				m_parent->setLogin(login);
				m_parent->setPassword(password);
			}
		};

		static inline char hex(char c)
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
		static inline std::string decode(const char* in, size_t in_len)
		{
			std::string out;
			out.reserve(in_len);

			for (size_t i = 0; i < in_len; ++i) {
				// go inside only, if there is enough space
				if (in[i] == '%' && (i < in_len - 2) &&
					isxdigit(in[i + 1]) && isxdigit(in[i + 2])) {
					unsigned char c = (hex(in[i + 1]) << 4) | hex(in[i + 2]);
					out += c;
					i += 2;
					continue;
				}
				out += in[i];
			}
			return out;
		}
		static inline std::string decode(const std::string& in)
		{
			return decode(in.c_str(), in.length());
		}
	public:
		explicit credentials_provider(const std::shared_ptr<server>& parent, const gui::credential_ui_ptr& auth_ui) : m_parent(parent), m_auth_ui(auth_ui) {}
		std::string getUsername() override { return m_parent->login(); }
		std::string getPassword() override { return m_parent->passwd(); }
		std::future<bool> authenticationRequested(const std::string& url, const std::string& realm) override
		{
			if (!m_auth_ui) {
				std::promise<bool> preset;
				auto future = preset.get_future();
				preset.set_value(false);
				return future;
			}
			// Jira Seraphim's realm seems to be URL-encoded server address:
			return m_auth_ui->ask_user(std::make_shared<server_credentials>(m_parent), url, decode(realm));
		}
	};

	server::server(const std::string& name, const std::string& login, const std::vector<uint8_t>& password, const std::string& url, const std::vector<search_def>& views, from_storage)
		: m_name(name)
		, m_login(login)
		, m_password(password)
		, m_url(url)
		, m_views(views)
		, m_db(url)
		, m_id(nextToken())
	{
		if (Uri{ url }.relative())
			m_url = "http://" + url;
	}

	server::server(const std::string& name, const std::string& login, const std::string& password, const std::string& url, const std::vector<search_def>& views)
		: m_name(name)
		, m_login(login)
		, m_password()
		, m_url(url)
		, m_views(views)
		, m_db(url)
		, m_id(nextToken())
	{
		if (Uri{ url }.relative())
			m_url = "http://" + url;

		if (!password.empty() && !secure::crypt({ password.begin(), password.end() }, m_password))
			throw std::bad_alloc();
	}

	void server::setPassword(const std::string& password)
	{
		m_password.clear();
		if (!password.empty() && !secure::crypt({ password.begin(), password.end() }, m_password))
			throw std::bad_alloc();
	}

	void server::onListenerAdded(const std::shared_ptr<server_listener>& listener)
	{
		size_t id = 0;
		for (auto& view : m_views) {
			if (view.loading())
				listener->onRefreshStarted(view.sessionId());
			++id;
		}
	}

	std::string server::passwd() const
	{
		std::vector<uint8_t> plain;
		if (!m_password.empty() && !secure::decrypt(m_password, plain))
			throw std::bad_alloc();

		return{ plain.begin(), plain.end() };
	}

	static std::string to_string(json::type type) {
		switch (type) {
		case json::NULLPTR: return "nullptr";
		case json::BOOL: return "boolean";
		case json::NUMBER: return "number";
		case json::INTEGER: return "integer";
		case json::FLOAT: return "float";
		case json::STRING: return "string";
		case json::VECTOR: return "array";
		case json::MAP: return "dictionary";
		}
		return std::to_string((int)type);
	}
	void server::loadFields(const std::shared_ptr<gui::document>& doc)
	{
		m_errors.clear();
		m_isLoadingFields = true;
		loadJSON(doc, "rest/api/2/field", [this](XHR* xhr, const json::value& doc) {
			m_db.reset_defs();

			ON_EXIT([this] {
				m_isLoadingFields = false;
				auto copy = m_viewsToRefresh;
				for (auto& pair : copy) {
					if (pair.second)
						refresh(pair.second, pair.first);
				}
			});

			if ((xhr->getStatus() / 100) != 2) {
				m_errors.emplace_back("Error loading fields: " + std::to_string(xhr->getStatus()) + " " + xhr->getStatusText());
			}

			if (!doc.is<json::vector>()) {
				m_errors.emplace_back("Returning document is not a json array: " + to_string(doc.get_type()));
				return;
			}

#if USE_ODS
			std::map<std::string, bool> failed;
#endif

			for (const auto& vfld : doc.as<json::vector>()) {
				if (!vfld.is<json::map>())
					continue;

				auto fld = vfld.as<json::map>();

				auto id = fld["id"].as_string();
				auto it = fld.find("name");
				auto display = it == fld.end() ? id : it->second.as_string();

				auto schema_it = fld.find("schema");
				if (schema_it == fld.end()) {
					if (id == "issuekey")
						m_db.update_name("key", display);
					continue;
				}

				auto schema = schema_it->second.as<json::map>()["type"].as_string();

				bool is_array = schema == "array";
				bool custom = fld["custom"].as<bool>();

				if (is_array)
					schema = schema_it->second.as<json::map>()["items"].as_string();

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

	void server::refresh(const std::shared_ptr<gui::document>& doc, uint32_t viewID)
	{
		auto it = std::find_if(std::begin(m_views), std::end(m_views), [viewID](const search_def& info) { return info.sessionId() == viewID; });
		if (it == m_views.end())
			return;

		auto doc_it = m_viewsToRefresh.find(viewID);
		if (it->loading() && (doc_it == m_viewsToRefresh.end() || !doc_it->second))
			return;

		if (!it->loading())
			emit([&, viewID](server_listener* listener) { listener->onRefreshStarted(viewID); });

		if (doc_it != m_viewsToRefresh.end())
			m_viewsToRefresh.erase(doc_it);
		it->startLoading();

		if (m_isLoadingFields) {
			m_viewsToRefresh[viewID] = doc;
			return;
		}

		auto thiz = shared_from_this();
		search(doc, viewID, [viewID, thiz](XHR* /*xhr*/, jira::report&& report) {
			auto fn = [viewID, thiz] {
				thiz->emit([&, viewID](server_listener* listener) { listener->onRefreshFinished(viewID); });
				auto it = std::find_if(std::begin(thiz->m_views), std::end(thiz->m_views),
					[viewID](const search_def& info) { return info.sessionId() == viewID; });
				if (it == thiz->m_views.end())
					return;
				it->endLoading();
			};
			ON_EXIT(fn);

			thiz->m_dataset = std::make_shared<jira::report>(std::move(report));
		}, [viewID, thiz](net::http::client::XmlHttpRequest* /*req*/, bool calculable, uint64_t content, uint64_t loaded) {
			thiz->emit([&, viewID](server_listener* listener) { listener->onProgress(viewID, calculable, content, loaded); });
		});

	}

	void server::refresh(const std::shared_ptr<gui::document>& doc)
	{
		for (auto& view : m_views)
			refresh(doc, view.sessionId());
	}

	void server::debugDump(std::ostream& o)
	{
		m_db.debug_dump(o);
	}

	void server::get(const std::shared_ptr<gui::document>& doc, const std::string & uri, const std::function<void(net::http::client::XmlHttpRequest*)>& onDone, const ONPROGRESS& progress, bool async)
	{
		using namespace net::http::client;

		auto xhr = doc->createXHR();
		xhr->onreadystatechange([xhr, onDone, this](XmlHttpRequest* req) {
			if (req->getReadyState() != XmlHttpRequest::DONE)
				return;

			auto& err = req->getError();
			if (!err.empty())
				m_errors.push_back(err);

			onDone(req);
			req->onreadystatechange(XmlHttpRequest::ONREADYSTATECHANGE{}); // clean up xhr 
			req->onprogress(XmlHttpRequest::ONPROGRESS{}); // clean up xhr 
		});

		if (progress)
			xhr->onprogress([xhr, progress](bool calculable, uint64_t content, uint64_t loaded) { progress(xhr.get(), calculable, content, loaded); });

		xhr->open(HTTP_GET, Uri::canonical(uri, url()).string(), async);
		xhr->setCredentials(std::make_shared<credentials_provider>(shared_from_this(), doc->authUI()));
		xhr->send();
	}

	void server::loadJSON(const std::shared_ptr<gui::document>& doc, const std::string& uri, const std::function<void(XHR*, const json::value&)>& response, const ONPROGRESS& progress, bool async)
	{
		using namespace net::http::client;
		get(doc, uri, [response](XmlHttpRequest* req) {
			if (req->getStatus() / 100 == 2) {
				auto text = req->getResponseText();
				auto length = req->getResponseTextLength();
				response(req, json::from_string(text, length));
			} else {
				response(req, json::value{});
			}
		}, progress, async);
	}

	void server::search(const std::shared_ptr<gui::document>& doc, const search_def& def, const std::function<void(XHR*, report&&)>& response, const ONPROGRESS& progress, bool async)
	{
		if (url().empty()) {
			m_errors.push_back("Trying to open an empty URL.");
			response(nullptr, report{});
			return;
		}

		auto& jql = def.jql().empty() ? search_def::standard.jql() : def.jql();
		auto& columns = def.columns().empty() ? search_def::standard.columns() : def.columns();
		auto joined = def.columns().empty() ? search_def::standard.columnsDescr() : def.columnsDescr();

		// special case: "summary" requires "parent" to be present.
		bool has_summary = false;
		for (auto& col : columns) {
			if (col == "summary") {
				has_summary = true;
				break;
			}
		}

		if (has_summary)
			joined += ",parent";

		Uri uri{ "rest/api/2/search" };
		uri.query(Uri::QueryBuilder{}.add("jql", jql).add("fields", joined).string());

		auto base = url();
		loadJSON(doc, uri.string(), [this, doc, response, columns, base](XHR* xhr, const json::value& data) {

			if ((xhr->getStatus() / 100) != 2) {
				m_errors.emplace_back("Error loading query reply: " + std::to_string(xhr->getStatus()) + " " + xhr->getStatusText());
				response(xhr, report{});
				return;
			}

			if (!data.is<json::map>()) {
				m_errors.emplace_back("Returning document is not a json dictionary: " + to_string(data.get_type()));
				response(xhr, report{});
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

				dataset.data.push_back(dataset.schema.visit(doc, issue["fields"], key, id));
			}

			response(xhr, std::move(dataset));
		}, progress, async);
	}

	void server::search(const std::shared_ptr<gui::document>& doc, uint32_t viewID, const std::function<void(XHR*, report&&)>& response, const ONPROGRESS& progress, bool async)
	{
		auto it = std::find_if(std::begin(m_views), std::end(m_views), [viewID](const search_def& info) { return info.sessionId() == viewID; });
		if (it == m_views.end())
			return;
		search(doc, *it, response, progress, async);
	}

	void server::forceLogin(const std::shared_ptr<gui::document>& doc, const std::function<void()>& then, bool async)
	{
		loadJSON(doc, "rest/auth/1/session", [then](net::http::client::XmlHttpRequest* req, const json::value&) {
			if (req->getStatus() / 100 == 2)
				then();
		}, { }, async);
	}

	class server_locator : public std::enable_shared_from_this<server_locator> {
		Uri m_uri;
		std::string m_path;
		std::shared_ptr<gui::document> m_doc;
		std::function<void(const server_info&)> m_cb;

	public:
		server_locator(const std::shared_ptr<gui::document>& doc, const Uri& uri, const std::function<void(const server_info&)>& cb)
			: m_uri { uri }
			, m_doc { doc }
			, m_cb { cb }
		{
			m_uri.query({ });
			m_path = m_uri.path();
			m_uri.path({ });
		}

		void next_try(bool async)
		{
			using namespace net::http::client;

			auto xhr = m_doc->createXHR();
			auto keep = shared_from_this();

			auto combined = Uri::canonical("rest/api/2/serverInfo"s, Uri::canonical(m_path, m_uri));

			xhr->onreadystatechange([xhr, keep, async, combined](XmlHttpRequest* req) {
				if (req->getReadyState() != XmlHttpRequest::DONE)
					return;

				keep->analyzeFromHeaders(req, combined, async);
				req->onreadystatechange({ }); // clean up xhr 
			});

			xhr->open(HTTP_HEAD, combined.string(), async);
			xhr->send();
		}

	private:
		void analyzeFromHeaders(net::http::client::XmlHttpRequest* req, const Uri& uri, bool async)
		{
			auto status = req->getStatus();
			if (status / 100 == 2) {
				using namespace net::http::client;

				auto xhr = m_doc->createXHR();
				auto keep = shared_from_this();


				xhr->onreadystatechange([xhr, keep, async](XmlHttpRequest* req) {
					if (req->getReadyState() != XmlHttpRequest::DONE)
						return;

					keep->analyze(req, async);
					req->onreadystatechange({ }); // clean up xhr 
				});

				xhr->open(HTTP_GET, uri.string(), async);
				xhr->send();

				return;
			}

			if (m_path.empty())
				return m_cb({ });
			m_path = fs::path{ m_path }.parent_path().string();

			next_try(async);
		}

		void analyze(net::http::client::XmlHttpRequest* req, bool async)
		{
			auto status = req->getStatus();
			if (status / 100 == 2)
				if (answer(req))
					return;

			if (m_path.empty())
				return m_cb({ });
			m_path = fs::path { m_path }.parent_path().string();

			next_try(async);
		}

		bool answer(net::http::client::XmlHttpRequest* req)
		{
			auto text = req->getResponseText();
			auto length = req->getResponseTextLength();
			auto val = json::from_string(text, length);

			server_info ret;

			if (val.is<json::map>()) {
				json::map obj { val };
				auto it = obj.find("baseUrl");
				if (it != obj.end() && it->second.is<std::string>()) {
					ret.baseUrl = Uri::canonical("", it->second.as<std::string>()).string();
					it = obj.find("serverTitle");
					if (it != obj.end() && it->second.is<std::string>()) {
						ret.serverTitle = it->second.as<std::string>();
					}
					m_cb(ret);

					return true;
				}
			}

			return false;
		}
	};

	/*static*/ void server::find_root(const std::shared_ptr<gui::document>& doc, const std::string& url, const std::function<void(const server_info&)>& cb)
	{
		if (!cb)
			return;

		auto locator = std::make_shared<server_locator>(doc, url, cb);
		locator->next_try(true);
	}
};

