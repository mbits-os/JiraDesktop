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

#include "jira/jira.hpp"
#include "types.hpp"
#include <net/uri.hpp>
#include <sstream>

namespace jira
{
	std::string record::issue_uri() const
	{
		return Uri::canonical("browse/" + m_key, m_uri).string();
	}

	void record::addVal(std::unique_ptr<value>&& field)
	{
		m_values.push_back(std::move(field));
	}

	std::string record::text(const std::string& sep) const
	{
		std::ostringstream o;
		bool first = true;
		for (auto&& value : m_values) {
			if (first) first = false;
			else o << sep;

			o << value->text();
		}

		return o.str();
	}

	std::string record::html(const std::string& sep) const
	{
		std::ostringstream o;
		bool first = true;
		for (auto&& value : m_values) {
			if (first) first = false;
			else o << sep;

			o << value->html();
		}

		return o.str();
	}

	record model::visit(const json::map& object, const std::string& key, const std::string& id) const
	{
		record out;
		out.uri(m_uri);
		out.issue_key(key);
		out.issue_id(id);

		for (auto& col : m_cols)
			out.addVal(col->visit(out, object));

		return out;
	}

	db::db(const std::string& uri)
		: m_uri(uri)
	{
		field<fields::key>("key");
		field<fields::string>("string");
		field<fields::resolution>("resolution");
		field<fields::summary>("summary");
		field<fields::user>("user");
		field<fields::icon>("icon");

		field_def("key", false, "key", "Key");
		field_def("summary", false, "summary", "Summary");
		field_def("description", false, "string", "Description");
		field_def("issuetype", false, "icon", "Issue Type");
		field_def("priority", false, "icon", "Priority");
		field_def("status", false, "icon", "Status");
		field_def("reporter", false, "user", "Reporter");
		field_def("creator", false, "user", "Creator");
		field_def("assignee", false, "user", "Assignee");
		field_def("resolution", false, "resolution", "Resolution");
	}

	void db::field_def(const std::string& id, bool is_array, const std::string& type, const std::string& display)
	{
		auto& ref = m_fields[id];
		ref.m_type = type;
		ref.m_display = display;
		ref.m_array = is_array;
	}

	std::unique_ptr<type> db::create(const std::string& id) const
	{
		auto it = m_fields.find(id);
		if (it == m_fields.end()) return nullptr;

		auto type = m_types.find(it->second.m_type);
		if (type == m_types.end()) return nullptr;

		auto out = type->second->create(id, it->second.m_display);
		if (it->second.m_array)
			return std::make_unique<fields::array>(id, it->second.m_display, std::move(out));
		return out;
	}

	model db::create_model(const std::vector<std::string>& names)
	{
		std::vector<std::unique_ptr<type>> cols;
		cols.reserve(names.size());
		for (auto name : names)
			cols.push_back(create(name));
		return{ std::move(cols), m_uri };
	}
};

