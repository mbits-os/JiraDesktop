/*
 * Copyright (C) 2014 midnightBITS
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
#include <sstream>
#include <cctype>
#include <limits>

namespace jira
{
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

	namespace fields {
		key::key(const std::string& id) : type(id) {}

		template <json::type type_id>
		json::value_t<type_id> either_or(const json::map& object, const std::string& key)
		{
			auto it = object.find(key);
			if (it == object.end() || !it->second.is<type_id>())
				return json::value_t<type_id>{};

			return it->second.as<type_id>();
		}

		template <json::type type_id, typename Arg>
		json::value_t<type_id> either_or(const json::map& object, const std::string& key, Arg&& arg)
		{
			auto it = object.find(key);
			if (it == object.end() || !it->second.is<type_id>())
				return json::value_t<type_id>{ std::forward<Arg>(arg) };

			return it->second.as<type_id>();
		}

		void key::visit(record& out, const json::map& /*object*/) const
		{
			values::span s;
			s
				.add<values::label>("[")
				.add<values::link>(out.issue_uri(), std::make_unique<values::label>(out.issue_key()))
				.add<values::label>("]");
			out.add<values::span>(s);
		}

		string::string(const std::string& id) : type(id) {}

		void string::visit(record& out, const json::map& object) const
		{

			auto it = object.find(id());
			if (it == object.end())
				return out.add<values::empty>();

			out.add<values::label>(it->second.as_string());
		}

		summary::summary(const std::string& id) : type(id) {}

		void summary::visit(record& out, const json::map& object) const
		{
			auto it = object.find(id());
			std::string label = "Untitled";
			if (it != object.end() && it->second.is<json::STRING>())
				label = "\"" + it->second.as_string() + "\"";

			out.add<values::link>(out.issue_uri(), std::make_unique<values::label>(label));
		}

		user::user(const std::string& id) : type(id) {}

		uint32_t stoui(const char* in, char& error)
		{
			uint32_t out = 0;

			if (in) {
				while (std::isdigit((uint8_t) *in)) {
					out *= 10;
					out += (*in++) - '0';
				}

				error = *in;
			}

			return out;
		}

		void user::visit(record& out, const json::map& object) const
		{
			auto it = object.find(id());
			if (it == object.end() || !it->second.is<json::MAP>())
				return out.add<values::empty>();

			json::map data{ it->second };
			auto active = either_or<json::BOOL>(data, "active", false);
			auto display = either_or<json::STRING>(data, "displayName", "?");
			auto email = either_or<json::STRING>(data, "emailAddress");
			auto login = either_or<json::STRING>(data, "name");
			std::map<uint32_t, std::string> avatar;

			auto vvavatar = data.find("avatarUrls");
			if (vvavatar != data.end() && vvavatar->second.is<json::MAP>()) {
				json::map vavatar{ vvavatar->second };
				for (auto&& pair : vavatar) {
					if (!pair.second.is<json::STRING>())
						continue;

					char error;
					auto size = stoui(pair.first.c_str(), error);

					if (size && (error == 0 || error == 'x' || error == 'X'))
						avatar[size] = pair.second.as<json::STRING>();
				}
			}

			out.add<values::user>(active, display, email, login, std::move(avatar));
		}

		icon::icon(const std::string& id) : type(id) {}

		void icon::visit(record& out, const json::map& object) const
		{
			auto it = object.find(id());
			if (it == object.end() || !it->second.is<json::MAP>())
				return out.add<values::empty>();

			json::map data{ it->second };
			auto uri = data["iconUrl"];
			auto name = either_or<json::STRING>(data, "name", "?");
			auto description = either_or<json::STRING>(data, "description");
			if (!uri.is<json::STRING>())
				return out.add<values::label>(name);

			out.add<values::icon>(uri.as_string(), name, description);
		}
	}

	namespace values {
		std::string to_xml(const std::string& text)
		{
			std::string out;
			out.reserve(text.length() * 12 / 10);
			for (auto c : text) {
				switch (c) {
				case '&': out += "&amp;"; break;
				case '<': out += "&lt;"; break;
				case '>': out += "&gt;"; break;
				case '"': out += "&quot;"; break;
				default: out.push_back(c);
				};
			}
			return out;
		}

		empty::empty()
		{
		}

		std::string empty::text() const
		{
			return{};
		}

		std::string empty::html() const
		{
			return "&nbsp;";
		}

		icon::icon(const std::string& uri, const std::string& title, const std::string& description)
			: m_uri(uri)
			, m_title(title)
			, m_description(description)
		{
		}

		std::string icon::text() const
		{
			return m_title;
		}

		std::string icon::html() const
		{
			std::ostringstream o;
			o << "<img src=\"" << to_xml(m_uri) << "\" title=\"" << to_xml(m_title);
			if (!m_description.empty())
				o << "\n" << to_xml(m_description);
			o << "\" style=\"width: 16px; height: 16px; border: none\">";
			return o.str();
		}

		user::user(bool active, const std::string& display, const std::string& email, const std::string& login, std::map<uint32_t, std::string>&& avatar)
			: m_active(active)
			, m_display(display)
			, m_email(email)
			, m_login(login)
			, m_avatar(std::move(avatar))
		{
		}

		std::string user::text() const
		{
			return m_display;
		}

		std::string user::html() const
		{
			constexpr size_t defSize = 16;
			std::string image;
			auto it = m_avatar.find(defSize);
			if (it != m_avatar.end())
				image = it->second;
			else {
				size_t delta = std::numeric_limits<size_t>::max();
				for (auto& pair : m_avatar) {
					size_t d = pair.first > defSize ? pair.first - defSize : defSize - pair.first;
					if (d < delta) {
						delta = d;
						image = pair.second;
					}
				}
			}

			if (image.empty())
				return to_xml(m_display);

			std::ostringstream o;
			o << "<img src=\"" << to_xml(image) << "\" title=\"" << to_xml(m_display + "\n" + m_email) << "\" style=\"width: 16px; height: 16px; border: none; border-radius: 3px\">";
			return o.str();
		}

		label::label(const std::string& text)
			: m_text(text)
		{
		}

		std::string label::text() const
		{
			return m_text;
		}

		std::string label::html() const
		{
			return to_xml(m_text);
		}

		link::link(const std::string& uri, std::unique_ptr<value>&& content)
			: m_uri(uri)
			, m_content(std::move(content))
		{
		}

		std::string link::text() const
		{
			return m_content->text();
		}

		std::string link::html() const
		{
			std::ostringstream o;
			o << "<a href=\"" << to_xml(m_uri) << "\">" << m_content->html() << "</a>";
			return o.str();
		}

		span::span()
		{
		}

		std::string span::text() const
		{
			std::ostringstream o;
			for (auto&& item : m_content)
				o << item->text();

			return o.str();
		}

		std::string span::html() const
		{
			std::ostringstream o;
			for (auto&& item : m_content)
				o << item->html();

			return o.str();
		}
	}

	record model::visit(const json::map& object, const std::string& key, const std::string& id) const
	{
		record out;
		out.uri(m_uri);
		out.issue_key(key);
		out.issue_id(id);

		for (auto& col : m_cols)
			col->visit(out, object);

		return out;
	}

	db::db(const std::string& uri)
		: m_uri(uri)
	{
		field<fields::key>("key");
		field<fields::string>("string");
		field<fields::summary>("summary");
		field<fields::user>("user");
		field<fields::icon>("icon");

		field_def("key", "key", "Key");
		field_def("summary", "summary", "Summary");
		field_def("description", "string", "Description");
		field_def("issuetype", "icon", "Issue Type");
		field_def("priority", "icon", "Priority");
		field_def("status", "icon", "Status");
		field_def("reporter", "user", "Reporter");
		field_def("creator", "user", "Creator");
		field_def("assignee", "user", "Assignee");
	}

	void db::field_def(const std::string& id, const std::string& type, const char* /*display*/)
	{
		m_fields[id] = type;
	}

	std::unique_ptr<type> db::create(const std::string& id) const
	{
		auto it = m_fields.find(id);
		if (it == m_fields.end()) return nullptr;

		auto type = m_types.find(it->second);
		if (type == m_types.end()) return nullptr;

		return type->second->create(id);
	}
};

