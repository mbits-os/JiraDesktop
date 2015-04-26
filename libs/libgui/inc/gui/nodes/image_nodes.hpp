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

#include <gui/nodes/node_base.hpp>
#include <gui/nodes/document_impl.hpp>
#include <gui/document.hpp>
#include <gui/image.hpp>

namespace gui {
	class image_cb
		: public image_ref_callback
		, public std::enable_shared_from_this<image_cb> {

	public:
		std::weak_ptr<node> parent;
		void onImageChange(image_ref*) override;
	};

	class icon_node : public node_base {
		std::shared_ptr<image_ref> m_image;
		std::shared_ptr<image_cb> m_cb;
	public:
		icon_node(const std::string& uri, const std::shared_ptr<image_ref>& image, const std::string& tooltip);
		icon_node(const icon_node& nod);
		~icon_node();
		void attach();
		void paintContents(painter* painter) override;
		size measureContents(painter* painter) override;

		bool isSupported(const std::shared_ptr<node>&) override;
		std::shared_ptr<node> cloneSelf() const override;
	};

	class user_node : public node_base {
		std::weak_ptr<document_impl> m_document;
		std::shared_ptr<image_ref> m_image;
		std::shared_ptr<image_cb> m_cb;
		std::map<uint32_t, std::string> m_urls;
		int m_selectedSize;
	public:
		user_node(const std::weak_ptr<document_impl>& document, std::map<uint32_t, std::string>&& avatar, const std::string& tooltip);
		user_node(const user_node&);
		~user_node();
		void paintContents(painter* painter) override;
		size measureContents(painter* painter) override;

		bool isSupported(const std::shared_ptr<node>&) override;
		std::shared_ptr<node> cloneSelf() const override;
	};
}