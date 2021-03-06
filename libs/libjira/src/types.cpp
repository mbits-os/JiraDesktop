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
#include <net/utf8.hpp>

namespace jira
{
	namespace fields {
		std::string utf8initial(const std::string& utf8) {
			return{ utf8.c_str(), utf::next_char(utf8.c_str()) };
		}

		key::key(const std::string& id, const std::string& title) : type(id, title) {}

		template <json::type type_id>
		std::decay_t<json::value_t<type_id>> either_or(const json::map& object, const std::string& key)
		{
			auto it = object.find(key);
			if (it == object.end() || !it->second.is<type_id>())
				return std::decay_t<json::value_t<type_id>>{};

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
		std::decay_t<json::value_t<type_id>> either_or(const json::map& object, const std::string& key, Arg&& arg)
		{
			auto it = object.find(key);
			if (it == object.end() || !it->second.is<type_id>())
				return std::decay_t<json::value_t<type_id>>{ std::forward<Arg>(arg) };

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

		std::shared_ptr<gui::node> key::visit(const std::shared_ptr<gui::document>& doc, const record& issue, const json::map& /*object*/) const
		{
			auto link = doc->createLink(issue.issue_uri());
			link->innerText(issue.issue_key());
			return std::move(link);
		}

		string::string(const std::string& id, const std::string& title) : type(id, title) {}

		std::shared_ptr<gui::node> string::visit(const std::shared_ptr<gui::document>& doc, const record& /*issue*/, const json::map& object) const
		{
			auto it = object.find(id());
			if (it == object.end())
				return doc->createElement(gui::elem::span);

			return doc->createText(it->second.as_string());
		}

		label::label(const std::string& id, const std::string& title) : type(id, title) {}

		std::shared_ptr<gui::node> label::visit(const std::shared_ptr<gui::document>& doc, const record& /*issue*/, const json::value& object) const
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
				auto node = doc->createElement(gui::elem::span);
				node->innerText(text);
				node->addClass("label");
				return node;
			}

			return doc->createElement(gui::elem::span);
		}

		resolution::resolution(const std::string& id, const std::string& title) : type(id, title) {}

		std::shared_ptr<gui::node> resolution::visit(const std::shared_ptr<gui::document>& doc, const record& /*issue*/, const json::map& object) const
		{
			auto it = object.find(id());
			if (it == object.end() || it->second.is<std::nullptr_t>()) {
				auto node = doc->createElement(gui::elem::span);
				node->innerText("Unresolved");
				node->addClass("none"); // font-style: italic; color: #555
				return std::move(node); 
			}

			if (it->second.is<std::string>())
				return doc->createText(it->second.as<std::string>());

			if (it->second.is<json::map>()) {
				json::map map{ it->second };
				auto name = either_or<std::string>(map, "name", "?");
				auto description = either_or<std::string>(map, "description");

				auto label = doc->createElement(gui::elem::span);
				label->innerText(name);
				label->setTooltip(description);
				return std::move(label);
			}

			auto node = doc->createElement(gui::elem::span);
			node->innerText("{!}"); // color:#E60026
			node->addClass("unexpected");
			return std::move(node);
		}

		summary::summary(const std::string& id, const std::string& title) : type(id, title) {}

		std::shared_ptr<gui::node> summary::visit(const std::shared_ptr<gui::document>& doc, const record& issue, const json::map& object) const
		{
			auto it = object.find(id());
			std::string label = "Untitled";
			if (it != object.end() && it->second.is<json::STRING>())
				label = it->second.as_string();

			auto link = doc->createLink(issue.issue_uri());
			link->innerText(label);

			it = object.find("parent");
			if (it != object.end() && it->second.is<json::MAP>()) {
				json::map parent{ it->second };
				it = parent.find("key");
				if (it != parent.end() && it->second.is<json::STRING>()) {
					auto key = it->second.as<json::STRING>();
					auto parent_link = doc->createLink(issue.issue_uri(key));
					parent_link->innerText(key);
					parent_link->addClass("parent-link");
					it = parent.find("fields");
					if (it != parent.end() && it->second.is<json::MAP>()) {
						json::map parent_fields{ it->second };
						it = parent_fields.find("summary");
						if (it != parent_fields.end() && it->second.is<json::STRING>())
							parent_link->setTooltip(it->second.as_string());
					}

					auto span = doc->createElement(gui::elem::span);
					span->appendChild(parent_link);
					span->appendChild(doc->createText("/"));
					span->appendChild(link);

					return span;
				}
			}

			return link;
		}

		user::user(const std::string& id, const std::string& title) : type(id, title)
		{
			if (!title.empty())
				m_title = utf8initial(title);
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

		std::shared_ptr<gui::node> user::visit(const std::shared_ptr<gui::document>& doc, const record& /*issue*/, const json::map& object) const
		{
			auto it = object.find(id());
			if (it == object.end() || !it->second.is<json::MAP>())
				return doc->createElement(gui::elem::span);

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
				m_title = utf8initial(title);
		}

		std::shared_ptr<gui::node> icon::visit(const std::shared_ptr<gui::document>& doc, const record& /*issue*/, const json::map& object) const
		{
			auto it = object.find(id());
			if (it == object.end() || !it->second.is<json::MAP>())
				return doc->createElement(gui::elem::span);

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

		array::array(const std::string& id, const std::string& title, const std::shared_ptr<type>& item, const std::string& sep)
			: type(id, title)
			, m_item(std::move(item))
			, m_sep(sep)
		{
		}

		std::shared_ptr<gui::node> array::visit(const std::shared_ptr<gui::document>& doc, const record& issue, const json::map& object) const
		{
			auto it = object.find(id());
			if (it == object.end() || !it->second.is<json::vector>()) {
				auto node = doc->createElement(gui::elem::span);
				node->innerText("None");
				node->addClass("none"); // font-style: italic; color: #555
				return std::move(node);
			}

			auto out = doc->createElement(gui::elem::span);

			bool first = true;
			auto items = it->second.as<json::vector>();
			for (auto&& item : items) {
				if (first) first = false;
				else out->appendChild(doc->createText(m_sep));

				auto val = m_item->visit(doc, issue, item);
				if (val)
					out->appendChild(std::move(val));
			}

			if (first) { // no items added to the span, return empty...
				auto node = doc->createElement(gui::elem::span);
				node->innerText("None");
				node->addClass("none"); // font-style: italic; color: #555
				return std::move(node);
			}

			return std::move(out);
		}
	}
};

