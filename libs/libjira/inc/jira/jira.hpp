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

#ifndef __JIRA_JIRA_HPP__
#define __JIRA_JIRA_HPP__

#include <json/json.hpp>
#include <gui/node.hpp>
#include <gui/document.hpp>
#include <memory>
#include <string>
#include <vector>
#include <map>

namespace jira
{
	class server;

	class record {
		std::shared_ptr<gui::node> m_row;

		std::string m_uri;
		std::string m_key;
		std::string m_id;

	public:
		record(const record&) = delete;
		record& operator=(const record&) = delete;
		record(record&&) = default;
		record& operator=(record&&) = default;
		record() = default;

		std::string issue_uri() const;
		std::string issue_uri(const std::string& key) const;
		void uri(const std::string& val) { m_uri = val; }
		void issue_key(const std::string& key) { m_key = key; }
		void issue_id(const std::string& id) { m_id = id; }
		const std::string& issue_key() const { return m_key; }
		const std::string& issue_id() const { return m_id; }

		void setRow(const std::shared_ptr<gui::node>&);
		const std::shared_ptr<gui::node>& getRow() const { return m_row; }
		void addVal(const std::shared_ptr<gui::node>&);

		const std::vector<std::shared_ptr<gui::node>>& values() const { return m_row->children(); }
	};

	struct type {
		type(const std::string& id, const std::string& title) : m_id(id), m_title(title) {}
		virtual ~type() {}
		virtual const std::string& id() const { return m_id; }
		virtual const std::string& title() const { return titleFull(); }
		virtual const std::string& titleFull() const { return m_title; }
		virtual std::shared_ptr<gui::node> visit(const std::shared_ptr<gui::document>& doc, const record& issue, const json::map& object) const = 0;
		virtual std::shared_ptr<gui::node> visit(const std::shared_ptr<gui::document>& doc, const record& issue, const json::value& value) const;
	private:
		std::string m_id;
		std::string m_title;
	};

	class db;
	class model {
		friend class db;
		std::string m_uri;
		std::vector<std::unique_ptr<type>> m_cols;
		model(std::vector<std::unique_ptr<type>>&& cols, const std::string& uri) : m_cols(std::move(cols)), m_uri(uri) {}
	public:
		model() = default;
		record visit(const std::shared_ptr<gui::document>& doc, const json::value& object, const std::string& key, const std::string& id) const;

		const std::vector<std::unique_ptr<type>>& cols() const { return m_cols; }
	};

	class db {

		struct creator {
			virtual ~creator() {}
			virtual std::unique_ptr<type> create(const std::string& id, const std::string& title) = 0;
		};

		template <typename T>
		struct creator_impl : creator {
			std::unique_ptr<type> create(const std::string& id, const std::string& title) override
			{
				return std::make_unique<T>(id, title);
			}
		};

		struct creator_descr {
			std::unique_ptr<creator> m_fn;
			std::string m_sep;
		};

		struct type_descr {
			std::string m_type;
			std::string m_display;
			bool m_array;
		};

		std::string m_uri;
		std::map<std::string, creator_descr> m_types;
		std::map<std::string, type_descr> m_fields;

		template <typename T>
		inline void field(const std::string& type, const std::string& sep = {})
		{
			auto it = m_types.find(type);
			creator_descr creator{ std::make_unique<creator_impl<T>>(), sep };
			if (it == m_types.end()) {
				m_types.emplace(type, std::move(creator));
				return;
			}
			it->second = std::move(creator);
		}

		std::unique_ptr<type> create(const std::string& id) const;
	public:
		db() = default;
		db(const std::string& uri);
		void reset_defs();
		void debug_dump(std::ostream&);
		model create_model(const std::vector<std::string>& names);
		bool field_def(const std::string& id, bool is_array, const std::string& type, const std::string& display);
		void update_name(const std::string& id, const std::string& display);
	};
}

#endif // __JIRA_JIRA_HPP__
