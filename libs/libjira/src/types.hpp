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

#ifndef __TYPES_HPP__
#define __TYPES_HPP__

#include "jira/jira.hpp"

namespace jira
{
	namespace fields {
		class key : public type {
		public:
			key(const std::string& id, const std::string& title);
			std::shared_ptr<gui::node> visit(const std::shared_ptr<gui::document>& doc, const record& issue, const json::map& /*object*/) const override;
		};

		class string : public type {
		public:
			string(const std::string& id, const std::string& title);
			std::shared_ptr<gui::node> visit(const std::shared_ptr<gui::document>& doc, const record& issue, const json::map& object) const override;
		};

		class label : public type {
		public:
			label(const std::string& id, const std::string& title);
			std::shared_ptr<gui::node> visit(const std::shared_ptr<gui::document>& /*doc*/, const record& /*issue*/, const json::map& /*object*/) const override { return{}; }
			std::shared_ptr<gui::node> visit(const std::shared_ptr<gui::document>& doc, const record& issue, const json::value& object) const override;
		};

		class resolution : public type {
		public:
			resolution(const std::string& id, const std::string& title);
			std::shared_ptr<gui::node> visit(const std::shared_ptr<gui::document>& doc, const record& issue, const json::map& object) const override;
		};

		class summary : public type {
		public:
			summary(const std::string & id, const std::string& title);
			std::shared_ptr<gui::node> visit(const std::shared_ptr<gui::document>& doc, const record& issue, const json::map& object) const override;
		};

		class user : public type {
			std::string m_title;
		public:
			user(const std::string & id, const std::string& title);
			std::shared_ptr<gui::node> visit(const std::shared_ptr<gui::document>& doc, const record& issue, const json::map& object) const override;
			const std::string& title() const override;
		};

		class icon : public type {
			std::string m_title;
		public:
			icon(const std::string& id, const std::string& title);
			std::shared_ptr<gui::node> visit(const std::shared_ptr<gui::document>& doc, const record& issue, const json::map& object) const override;
			const std::string& title() const override;
		};

		class array : public type {
			std::shared_ptr<type> m_item;
			std::string m_sep;
		public:
			array(const std::string& id, const std::string& title, const std::shared_ptr<type>& item, const std::string& sep);
			std::shared_ptr<gui::node> visit(const std::shared_ptr<gui::document>& doc, const record& issue, const json::map& object) const override;
		};
	}
}

#endif // __JIRA_JIRA_HPP__
