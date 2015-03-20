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
#include "values.hpp"
#include <sstream>
#include <limits>

namespace jira
{
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
				case '\n': out += "&#xd;&#xa;"; break;
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
				o << to_xml("\n" + m_description);
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
};

