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

	void record::addVal(std::unique_ptr<node>&& field)
	{
		m_values.push_back(std::move(field));
	}

	std::unique_ptr<node> type::visit(document* doc, const record& issue, const json::value& value) const
	{
		if (!value.is<json::map>())
			return nullptr;

		return visit(doc, issue, value.as<json::map>());
	}

	record model::visit(document* doc, const json::value& object, const std::string& key, const std::string& id) const
	{
		record out;
		out.uri(m_uri);
		out.issue_key(key);
		out.issue_id(id);

		for (auto& col : m_cols) {
			auto val = col->visit(doc, out, object);
			if (val)
				out.addVal(std::move(val));
		}

		return out;
	}

	db::db(const std::string& uri)
		: m_uri(uri)
	{
		field<fields::key>("key");
		field<fields::string>("string");
		field<fields::label>("labels", " ");
		field<fields::resolution>("resolution");
		field<fields::summary>("summary");
		field<fields::user>("user");
		field<fields::icon>("icon");
		field<fields::icon>("issuetype");
		field<fields::icon>("priority");
		field<fields::icon>("status");

		reset_defs();
	}

	void db::reset_defs()
	{
		m_fields.clear();
		field_def("key", false, "key", "Key");
	}

	void db::debug_dump(std::ostream& o)
	{
		for (auto& type : m_types)
			o << type.first << "\n";

		o << "\n";

		for (auto& fld : m_fields) {
			o << fld.first << " (";
			if (fld.second.m_array)
				o << "array of ";
			o << fld.second.m_type << ") " << json::value{ fld.second.m_display }.to_string() << "\n";
		}
	}

	bool db::field_def(const std::string& id, bool is_array, const std::string& type, const std::string& display)
	{
		auto it = m_types.find(type);
		if (it == m_types.end())
			return false;

		auto& ref = m_fields[id];
		ref.m_type = type;
		ref.m_display = display;
		ref.m_array = is_array;
		return true;
	}

	std::unique_ptr<type> db::create(const std::string& id) const
	{
		auto it = m_fields.find(id);
		if (it == m_fields.end())
			return nullptr;

		auto type = m_types.find(it->second.m_type);
		if (type == m_types.end())
			return nullptr;

		auto out = type->second.m_fn->create(id, it->second.m_display);
		if (it->second.m_array)
			return std::make_unique<fields::array>(id, it->second.m_display, std::move(out), type->second.m_sep.empty() ? ", " : type->second.m_sep);
		return out;
	}

	model db::create_model(const std::vector<std::string>& names)
	{
		std::vector<std::unique_ptr<type>> cols;
		cols.reserve(names.size());
		for (auto name : names) {
			auto field = create(name);
			if (field)
				cols.push_back(std::move(field));
		}
		return{ std::move(cols), m_uri };
	}
};

