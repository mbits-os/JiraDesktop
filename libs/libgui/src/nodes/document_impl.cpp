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

#include <gui/nodes/document_impl.hpp>

#include <gui/nodes/image_nodes.hpp>
#include <gui/nodes/link_node.hpp>
#include <gui/nodes/text_node.hpp>
#include <gui/nodes/block_node.hpp>
#include <gui/nodes/span_node.hpp>
#include <gui/nodes/table_node.hpp>
#include <gui/nodes/caption_row_node.hpp>
#include <gui/nodes/row_node.hpp>

#include <assert.h>
#define ASSERT(x) assert(x)

namespace gui {
	using namespace styles::literals;

	std::shared_ptr<document> document::make_doc(const std::shared_ptr<image_creator>& imageCreator, const std::shared_ptr<xhr_constructor>& xhrCtor)
	{
		return std::make_shared<document_impl>(imageCreator, xhrCtor);
	}

	document_impl::document_impl(const std::shared_ptr<image_creator>& imageCreator, const std::shared_ptr<xhr_constructor>& xhrCtor)
		: m_creator(imageCreator)
		, m_xhr(xhrCtor)
	{
	}

	std::shared_ptr<node> document_impl::createIcon(const std::string& uri, const std::string& text, const std::string& description)
	{
		auto image = createImage(uri);
		auto tooltip = text;
		if (text.empty() || description.empty())
			tooltip += description;
		else
			tooltip += "\n" + description;

		auto icon = std::make_shared<icon_node>(uri, image, tooltip);
		icon->attach();
		return icon;
	}

	std::shared_ptr<node> document_impl::createUser(bool /*active*/, const std::string& display, const std::string& email, const std::string& /*login*/, std::map<uint32_t, std::string>&& avatar)
	{
		if (avatar.empty()) {
			auto node = createText(display);
			if (!email.empty())
				node->setTooltip(email);
			return std::move(node);
		}

		auto tooltip = display;
		if (display.empty() || email.empty())
			tooltip += email;
		else
			tooltip += "\n" + email;

		return std::make_shared<user_node>(shared_from_this(), std::move(avatar), tooltip);
	}

	std::shared_ptr<node> document_impl::createLink(const std::string& href)
	{
		return std::make_shared<link_node>(href);
	}

	std::shared_ptr<node> document_impl::createText(const std::string& text)
	{
		return std::make_shared<text_node>(text);
	}

	std::shared_ptr<image_ref> document_impl::createImage(const std::string& uri)
	{
		std::lock_guard<std::mutex> lock(m_guard);
		auto it = m_cache.lower_bound(uri);
		if (it != m_cache.end() && it->first == uri)
			return it->second;

		ASSERT(!!m_creator);
		auto image = m_creator->create(uri);
		m_cache.insert(it, std::make_pair(uri, image));
		return image;
	}

	std::shared_ptr<node> document_impl::createElement(const elem name)
	{
		switch (name) {
		case elem::block: return std::make_shared<block_node>(name);
		case elem::header: return std::make_shared<span_node>(name);
		case elem::span: return std::make_shared<span_node>(name);
		case elem::table: return std::make_shared<table_node>();
		case elem::table_caption: return std::make_shared<caption_row_node>();
		case elem::table_head: return std::make_shared<row_node>(name);
		case elem::table_row: return std::make_shared<row_node>(name);
		case elem::th: return std::make_shared<span_node>(name);
		case elem::td: return std::make_shared<span_node>(name);
		};

		return{};
	}

	net::http::client::XmlHttpRequestPtr document_impl::createXHR()
	{
		return m_xhr->create();
	}
}
