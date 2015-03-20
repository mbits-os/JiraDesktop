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
#include "types.hpp"
#include "values.hpp"
#include <cctype>

namespace jira
{
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
			out.add<values::span>(std::move(s));
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
};

