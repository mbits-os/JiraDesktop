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

#ifdef max
#undef max
#endif

#include <gui/nodes/image_nodes.hpp>
#include <limits>

#include <assert.h>
#define ASSERT(x) assert(x)

namespace gui {
	void image_cb::onImageChange(image_ref*)
	{
		auto par = parent.lock();
		if (par)
			par->invalidate();
	}

	icon_node::icon_node(const std::string& uri, const std::shared_ptr<image_ref>& image, const std::string& tooltip)
		: node_base(elem::image)
		, m_image(image)
		, m_cb(std::make_shared<image_cb>())
	{
		m_data[Attr::Href] = uri;
		node_base::setTooltip(tooltip);
		m_box.size = { 16_px, 16_px };
	}

	icon_node::icon_node(const icon_node& nod) = default;

	icon_node::~icon_node()
	{
		m_image->unregisterListener(m_cb);
		m_cb->parent.reset();
	}

	void icon_node::attach()
	{
		m_cb->parent = shared_from_this();
		m_image->registerListener(m_cb);
	}

	void icon_node::paintContents(painter* painter)
	{
		painter->paintImage(m_image.get(), 16_px, 16_px);
	}

	size icon_node::measureContents(painter*)
	{
		m_contentBaseline = 16_px;
		return{ 16_px, 16_px };
	}

	bool icon_node::isSupported(const std::shared_ptr<node>&)
	{
		return false;
	}

	std::shared_ptr<node> icon_node::cloneSelf() const
	{
		return cloneDetach(std::make_shared<icon_node>(*this));
	}

	user_node::user_node(const std::weak_ptr<document_impl>& document, std::map<uint32_t, std::string>&& avatar, const std::string& tooltip)
		: node_base(elem::icon)
		, m_document(document)
		, m_cb(std::make_shared<image_cb>())
		, m_urls(std::move(avatar))
		, m_selectedSize(0)
	{
		node_base::setTooltip(tooltip);
		m_box.size = { 16_px, 16_px };
	}

	user_node::user_node(const user_node&) = default;

	user_node::~user_node()
	{
		if (m_image)
			m_image->unregisterListener(m_cb);
		m_cb->parent.reset();
	}

	void user_node::paintContents(painter* painter)
	{
		painter->paintImage(m_image.get(), 16_px, 16_px);
	}

	size user_node::measureContents(painter* painter)
	{
		auto size = 16_px;
		m_contentBaseline = size;

		internalSetSize(size, size);
		auto scaled = (size_t)(painter->trueZoom().scaleL(size.value()));
		auto selected = 0;

		std::string image;
		auto it = m_urls.find(scaled);
		if (it != m_urls.end()) {
			image = it->second;
			selected = it->first;
		}
		else {
			size_t delta = std::numeric_limits<size_t>::max();
			for (auto& pair : m_urls) {
				if (pair.first < scaled)
					continue;

				size_t d = pair.first - scaled;
				if (d < delta) {
					delta = d;
					image = pair.second;
					selected = pair.first;
				}
			}
			for (auto& pair : m_urls) {
				if (pair.first > scaled)
					continue;

				size_t d = scaled - pair.first;
				if (d < delta) {
					delta = d;
					image = pair.second;
					selected = pair.first;
				}
			}
		}

		size = m_box.size.width;

		if (selected == m_selectedSize)
			return{ size, size };

		m_selectedSize = selected;

		if (m_image)
			m_image->unregisterListener(m_cb);
		m_image.reset();

		m_cb->parent = shared_from_this();
		auto doc = m_document.lock();
		if (doc)
			m_image = doc->createImage(image);

		if (m_image)
			m_image->registerListener(m_cb);

		return{ size, size };
	}

	bool user_node::isSupported(const std::shared_ptr<node>&)
	{
		return false;
	}

	std::shared_ptr<node> user_node::cloneSelf() const
	{
		return cloneDetach(std::make_shared<user_node>(*this));
	}
}
