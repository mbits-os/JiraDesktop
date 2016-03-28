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
#include <gui/listeners.hpp>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <chrono>
#include <limits>

#ifdef max
#undef max
#endif

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
		std::string m_title;
		std::string m_jql;
		std::vector<std::string> m_columns;
		std::chrono::milliseconds m_timeout{ std::chrono::milliseconds::max() };
		uint32_t m_id;
		bool m_isLoadingView = false;
	public:
		search_def();
		search_def(const search_def&);
		search_def(search_def&&);
		search_def& operator=(const search_def&);
		search_def& operator=(search_def&&);
		search_def(const std::string& title, const std::string& jql, const std::string& columnsDescr, std::chrono::milliseconds timeout);
		search_def(const std::string& title, const std::string& jql, const std::vector<std::string>& columns, std::chrono::milliseconds timeout);
		uint32_t sessionId() const { return m_id; }
		const std::string& title() const { return m_title; }
		const std::string& jql() const { return m_jql; }
		const std::vector<std::string>& columns() const { return m_columns; }
		std::string columnsDescr() const;
		std::chrono::milliseconds timeout() const { return m_timeout; }
		bool loading() const { return m_isLoadingView; }
		void startLoading() { m_isLoadingView = true; }
		void endLoading() { m_isLoadingView = false; }

		static const search_def standard;
	};

	struct server_listener {
		virtual ~server_listener() {}
		virtual void onRefreshStarted(uint32_t viewID) = 0;
		virtual void onProgress(uint32_t viewID, bool calculable, uint64_t content, uint64_t loaded) = 0;
		virtual void onRefreshFinished(uint32_t viewID) = 0;
	};

	struct server_info {
		std::string baseUrl;
		std::string serverTitle;
	};

	class server : public listeners<server_listener, server>, public std::enable_shared_from_this<server> {
		std::string m_name;
		std::string m_login;
		std::vector<uint8_t> m_password;
		std::string m_url;
		uint32_t m_id;

		std::atomic<bool> m_isLoadingFields { false };
		std::map<uint32_t, std::shared_ptr<gui::document>> m_viewsToRefresh;

		std::vector<search_def> m_views;
		std::shared_ptr<report> m_dataset;
		std::vector<std::string> m_errors;

		db m_db;

		std::string passwd() const;

		friend class listeners<server_listener, server>;

		void onListenerAdded(const std::shared_ptr<server_listener>& listener) /*override*/;

		class credentials_provider;
	public:
		enum from_storage { stored };

		using XHR =  net::http::client::XmlHttpRequest;

		using ONPROGRESS = std::function<void(XHR*, bool, uint64_t, uint64_t)>;

		server() = default;
		server(const std::string& name, const std::string& login, const std::vector<uint8_t>& password, const std::string& url, const std::vector<search_def>& views, from_storage);
		server(const std::string& name, const std::string& login, const std::string& password, const std::string& url, const std::vector<search_def>& views);
		const std::vector<uint8_t>& password() const { return m_password; }
		const std::string& name() const { return m_name; }
		const std::string& displayName() const { return m_name.empty() ? m_url : m_name; }
		const std::string& login() const { return m_login; }
		const std::string& url() const { return m_url; }
		const std::vector<search_def>& views() const { return m_views; }
		uint32_t sessionId() const { return m_id; }
		const std::vector<std::string>& errors() const { return m_errors; }

		void setPassword(const std::string& password);
		void setName(const std::string& name) { m_name = name; }
		void setLogin(const std::string& login) { m_login = login; }
		void setUrl(const std::string& url) { m_url = url; }
		void setViews(const std::vector<search_def>& views) { m_views = views; }

		void loadFields(const std::shared_ptr<gui::document>&);
		void refresh(const std::shared_ptr<gui::document>&, uint32_t);
		void refresh(const std::shared_ptr<gui::document>&);
		const std::shared_ptr<report>& dataset() const { return m_dataset; }
		void debugDump(std::ostream&);
		void get(const std::shared_ptr<gui::document>&, const std::string& uri, const std::function<void(XHR*)>& onDone, const ONPROGRESS& progress = {}, bool async = true);
		void loadJSON(const std::shared_ptr<gui::document>&, const std::string& uri, const std::function<void (XHR*, const json::value&)>& response, const ONPROGRESS& progress = {}, bool async = true);
		void search(const std::shared_ptr<gui::document>& doc, const search_def& def, const std::function<void(XHR*, report&&)>& response, const ONPROGRESS& progress = {}, bool async = true);
		void search(const std::shared_ptr<gui::document>& doc, uint32_t id, const std::function<void(XHR*, report&&)>& response, const ONPROGRESS& progress = {}, bool async = true);
		void forceLogin(const std::shared_ptr<gui::document>& doc, const std::function<void()>& then, bool async = true);

		static void find_root(const std::shared_ptr<gui::document>& doc, const std::string& url, const std::function<void(const server_info&)>& cb);
	};
}

#endif // __JIRA_SERVER_HPP__
