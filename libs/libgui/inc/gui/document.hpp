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

#pragma once

#include <gui/node.hpp>
#include <net/xhr.hpp>

namespace gui {
	struct image_creator {
		virtual ~image_creator() {}
		virtual std::shared_ptr<image_ref> create(const std::string&) = 0;
	};

	struct xhr_constructor {
		virtual ~xhr_constructor() {}
		virtual net::http::client::XmlHttpRequestPtr create() = 0;
	};

	struct credential_ui {
		struct owner {
			virtual ~owner() {}
			virtual std::string key() const = 0;
			virtual std::string get_username() const = 0;
			virtual std::string get_password() const = 0;
			virtual void set_credentials(const std::string& login, const std::string& password) = 0;
		};

		virtual ~credential_ui() {}
		virtual std::future<bool> ask_user(const std::shared_ptr<owner>& owner, const std::string& url, const std::string& realm) = 0;
	};

	using credential_ui_ptr = std::shared_ptr<credential_ui>;

	struct document {
		static std::shared_ptr<document> make_doc(const std::shared_ptr<image_creator>&, const std::shared_ptr<xhr_constructor>&, const credential_ui_ptr&);

		virtual ~document() {}
		virtual std::shared_ptr<node> createIcon(const std::string& uri, const std::string& text, const std::string& description) = 0;
		virtual std::shared_ptr<node> createUser(bool active, const std::string& display, const std::string& email, const std::string& login, std::map<uint32_t, std::string>&& avatar) = 0;
		virtual std::shared_ptr<node> createLink(const std::string& href) = 0;
		virtual std::shared_ptr<node> createText(const std::string& text) = 0;
		virtual std::shared_ptr<node> createElement(const elem name) = 0;
		virtual net::http::client::XmlHttpRequestPtr createXHR() = 0;
		virtual credential_ui_ptr authUI() = 0;
	};
};