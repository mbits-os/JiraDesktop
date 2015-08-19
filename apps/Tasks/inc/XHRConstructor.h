#pragma once

#include <gui/document.hpp>

class XHRConstructor : public gui::xhr_constructor {
	std::weak_ptr<gui::document> m_doc;

public:
	net::http::client::XmlHttpRequestPtr create();
	void setDoc(const std::shared_ptr<gui::document>& doc) { m_doc = doc; }
};
