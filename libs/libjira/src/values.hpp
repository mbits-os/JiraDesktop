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

#ifndef __VALUES_HPP__
#define __VALUES_HPP__

#include <jira/jira.hpp>

namespace jira
{
	namespace values {
		class empty : public value {
		public:
			empty();
			std::string text() const override;
			std::string html() const override;
		};

		class icon : public value {
			std::string m_uri;
			std::string m_title;
			std::string m_description;
		public:
			icon(const std::string& uri, const std::string& title, const std::string& description);
			std::string text() const override;
			std::string html() const override;
		};

		class user : public value {
			bool m_active;
			std::string m_display;
			std::string m_email;
			std::string m_login;
			std::map<uint32_t, std::string> m_avatar;
		public:
			user(bool active, const std::string& display, const std::string& email, const std::string& login, std::map<uint32_t, std::string>&& avatar);
			std::string text() const override;
			std::string html() const override;
		};

		class label : public value {
			std::string m_text;
		public:
			label(const std::string& text);
			std::string text() const override;
			std::string html() const override;
		};

		class link : public value {
			std::string m_uri;
			std::unique_ptr<value> m_content;
		public:
			link(const std::string& uri, std::unique_ptr<value>&& content);
			std::string text() const override;
			std::string html() const override;
		};

		class span : public value {
			std::vector<std::unique_ptr<value>> m_content;
		public:
			template <typename T, typename... Args>
			span& add(Args&&... args)
			{
				m_content.push_back(std::make_unique<T>(std::forward<Args>(args)...));
				return *this;
			}

			std::string text() const override;
			std::string html() const override;
		};
	};
}

#endif // __VALUES_HPP__
