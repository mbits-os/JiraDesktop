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

#include <gui/painter.hpp>
#include <gui/styles.hpp>
#include <memory>
#include <vector>

namespace gui {
	struct node {
		virtual void addChild(const std::shared_ptr<node>& child) = 0;
		virtual std::shared_ptr<node> insertBefore(const std::shared_ptr<node>& newChild, const std::shared_ptr<node>& refChild) = 0;
		virtual std::shared_ptr<node> replaceChild(const std::shared_ptr<node>& newChild, const std::shared_ptr<node>& oldChild) = 0;
		virtual std::shared_ptr<node> removeChild (const std::shared_ptr<node>& oldChild) = 0;
		virtual std::shared_ptr<node> appendChild (const std::shared_ptr<node>& newChild) = 0;
		virtual bool hasChildNodes() const = 0;
		virtual std::shared_ptr<node> cloneNode(bool deep) const = 0;
		virtual const std::vector<std::shared_ptr<node>>& children() const = 0;

		virtual elem getNodeName() const = 0;
		virtual void addClass(const std::string& name) = 0;
		virtual void removeClass(const std::string& name) = 0;
		virtual bool hasClass(const std::string& name) const = 0;
		virtual const std::vector<std::string>& getClassNames() const = 0;
		virtual std::string text() const = 0;
		virtual void paint(painter* painter) = 0;
		virtual void measure(painter* painter) = 0;
		virtual void setPosition(const pixels& x, const pixels& y) = 0;
		virtual void setSize(const pixels& width, const pixels& height) = 0;
		virtual point getPosition() = 0;
		virtual point getAbsolutePos() = 0;
		virtual size getSize() = 0;
		virtual pixels getBaseline() = 0;

		virtual std::shared_ptr<node> getParent() const = 0;
		virtual void setParent(const std::shared_ptr<node>&) = 0;
		virtual void invalidate() = 0;
		virtual void invalidate(const point& pt, const size& size) = 0;

		virtual std::shared_ptr<node> nodeFromPoint(const pixels& x, const pixels& y) = 0;
		virtual void setHovered(bool hovered) = 0;
		virtual bool getHovered() const = 0;
		virtual void setActive(bool active) = 0;
		virtual bool getActive() const = 0;
		virtual void activate() = 0;
		virtual gui::pointer getCursor() const = 0;

		virtual void setTooltip(const std::string& text) = 0;
		virtual bool hasTooltip() const = 0;
		virtual const std::string& getTooltip() const = 0;
		virtual void innerText(const std::string& text) = 0;

		virtual std::shared_ptr<styles::rule_storage> calculatedStyle() const = 0;
		virtual std::shared_ptr<styles::rule_storage> normalCalculatedStyles() const = 0;
		virtual std::shared_ptr<styles::stylesheet> styles() const = 0;
		virtual void applyStyles(const std::shared_ptr<styles::stylesheet>& stylesheet) = 0;
	};
};