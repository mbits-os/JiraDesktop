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
#include <gui/document.hpp>
#include <mutex>

namespace gui {
	class document_impl : public document, public std::enable_shared_from_this<document_impl> {
		std::shared_ptr<node> createIcon(const std::string& uri, const std::string& text, const std::string& description) override;
		std::shared_ptr<node> createUser(bool active, const std::string& display, const std::string& email, const std::string& login, std::map<uint32_t, std::string>&& avatar) override;
		std::shared_ptr<node> createLink(const std::string& href) override;
		std::shared_ptr<node> createText(const std::string& text) override;
		std::shared_ptr<node> createElement(const elem name) override;

		std::mutex m_guard;
		std::map<std::string, std::shared_ptr<image_ref>> m_cache;
		std::shared_ptr<image_creator> m_creator;

	public:
		explicit document_impl(const std::shared_ptr<image_creator>&);

		std::shared_ptr<image_ref> createImage(const std::string& uri);
	};
}
