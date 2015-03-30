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
#include <cctype>

namespace jira
{
	namespace fields {
		key::key(const std::string& id, const std::string& title) : type(id, title) {}

		template <json::type type_id>
		json::value_t<type_id> either_or(const json::map& object, const std::string& key)
		{
			auto it = object.find(key);
			if (it == object.end() || !it->second.is<type_id>())
				return json::value_t<type_id>{};

			return it->second.as<type_id>();
		}

		template <typename T>
		T either_or(const json::map& object, const std::string& key)
		{
			auto it = object.find(key);
			if (it == object.end() || !it->second.is<T>())
				return T{};

			return T{ it->second.as<T>() };
		}

		template <json::type type_id, typename Arg>
		json::value_t<type_id> either_or(const json::map& object, const std::string& key, Arg&& arg)
		{
			auto it = object.find(key);
			if (it == object.end() || !it->second.is<type_id>())
				return json::value_t<type_id>{ std::forward<Arg>(arg) };

			return it->second.as<type_id>();
		}

		template <typename T, typename Arg>
		T either_or(const json::map& object, const std::string& key, Arg&& arg)
		{
			auto it = object.find(key);
			if (it == object.end() || !it->second.is<T>())
				return T{ std::forward<Arg>(arg) };

			return T{ it->second.as<T>() };
		}

		std::unique_ptr<node> key::visit(document* doc, const record& issue, const json::map& /*object*/) const
		{
			auto link = doc->createLink(issue.issue_uri());
			link->addChild(doc->createText(issue.issue_key()));
			return std::move(link);
		}

		string::string(const std::string& id, const std::string& title) : type(id, title) {}

		std::unique_ptr<node> string::visit(document* doc, const record& /*issue*/, const json::map& object) const
		{
			auto it = object.find(id());
			if (it == object.end())
				return nullptr;

			return doc->createText(it->second.as_string());
		}

		label::label(const std::string& id, const std::string& title) : type(id, title) {}

		std::unique_ptr<node> label::visit(document* doc, const record& /*issue*/, const json::value& object) const
		{
			std::string text;
			if (object.is<std::string>())
				text = object.as<std::string>();
			else if (object.is<json::map>()) {
				auto map = object.as<json::map>();
				auto it = map.find(id());
				if (it != map.end())
					text = it->second.as_string();
			}

			if (!text.empty()) {
				// TODO: styling - background:#f5f5f5;border:1px solid #ccc;border-radius:3.01px;display:inline-block;padding:1px 5px; margin:0 3px 0 0;
				return doc->createText(text);
			}

			return nullptr;
		}

		resolution::resolution(const std::string& id, const std::string& title) : type(id, title) {}

		std::unique_ptr<node> resolution::visit(document* doc, const record& /*issue*/, const json::map& object) const
		{
			auto it = object.find(id());
			if (it == object.end() || it->second.is<nullptr_t>())
				return doc->createText("Unresolved"); // font-style: italic; color: #555

			if (it->second.is<std::string>())
				return doc->createText(it->second.as<std::string>());

			if (it->second.is<json::map>()) {
				json::map map{ it->second };
				auto name = either_or<std::string>(map, "name", "?");
				auto description = either_or<std::string>(map, "description");

				auto label = doc->createText(name);
				label->setTooltip(description);
				return std::move(label);
			}

			return doc->createText("{!}"); // color:#E60026
		}

		summary::summary(const std::string& id, const std::string& title) : type(id, title) {}

		std::unique_ptr<node> summary::visit(document* doc, const record& issue, const json::map& object) const
		{
			auto it = object.find(id());
			std::string label = "Untitled";
			if (it != object.end() && it->second.is<json::STRING>())
				label = it->second.as_string();

			auto link = doc->createLink(issue.issue_uri());
			link->addChild(doc->createText(label));
			return std::move(link);
		}

		user::user(const std::string& id, const std::string& title) : type(id, title)
		{
			if (!title.empty())
				m_title = title.substr(0, 1);
		}

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

		std::unique_ptr<node> user::visit(document* doc, const record& /*issue*/, const json::map& object) const
		{
			auto it = object.find(id());
			if (it == object.end() || !it->second.is<json::MAP>())
				return nullptr;

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

			return doc->createUser(active, display, email, login, std::move(avatar));
		}

		const std::string& user::title() const
		{
			return m_title;
		}

		icon::icon(const std::string& id, const std::string& title) : type(id, title)
		{
			if (!title.empty())
				m_title = title.substr(0, 1);
		}

		std::unique_ptr<node> icon::visit(document* doc, const record& /*issue*/, const json::map& object) const
		{
			auto it = object.find(id());
			if (it == object.end() || !it->second.is<json::MAP>())
				return nullptr;

			json::map data{ it->second };
			auto uri = data["iconUrl"];
			auto name = either_or<json::STRING>(data, "name", "?");
			auto description = either_or<json::STRING>(data, "description");
			if (!uri.is<json::STRING>())
				return doc->createText(name);

			return doc->createIcon(uri.as_string(), name, description);
		}

		const std::string& icon::title() const
		{
			return m_title;
		}

		array::array(const std::string& id, const std::string& title, std::unique_ptr<type>&& item, const std::string& sep)
			: type(id, title)
			, m_item(std::move(item))
			, m_sep(sep)
		{
		}

		std::unique_ptr<node> array::visit(document* doc, const record& issue, const json::map& object) const
		{
			auto it = object.find(id());
			if (it == object.end() || !it->second.is<json::vector>())
				return doc->createText("None"); // font-style: italic; color: #555

			auto out = doc->createSpan();

			bool first = true;
			auto items = it->second.as<json::vector>();
			for (auto&& item : items) {
				if (first) first = false;
				else out->addChild(doc->createText(m_sep));

				auto val = m_item->visit(doc, issue, item);
				if (val)
					out->addChild(std::move(val));
			}
			if (first) // no items added to the span, return empty...
				return doc->createText("None"); // font-style: italic; color: #555

			return std::move(out);
		}
	}
};

