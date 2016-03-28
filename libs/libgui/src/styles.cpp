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
#include <gui/styles.hpp>
#include <gui/node.hpp>

namespace styles {
	bool selector::selects(const gui::node* node) const
	{
		if (!maySelect(node))
			return false;

		switch (m_pseudoClass) {
		case pseudo::hover:
			if (!node->getHovered())
				return false;
			break;
		case pseudo::active:
			if (!node->getActive())
				return false;
			break;
		};

		return true;
	}

	// same as selects, but doesn't take pseudo classes into account
	bool selector::maySelect(const gui::node* node) const
	{
		if (!node)
			return false;

		if (m_elemName != gui::elem::unspecified && m_elemName != node->getNodeName())
			return false;

		for (auto& cl : m_classes) {
			if (!node->hasClass(cl))
				return false;
		}

		return true;
	}
};