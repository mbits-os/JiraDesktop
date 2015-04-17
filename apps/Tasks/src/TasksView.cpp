// TasksView.cpp : implementation of the CTasksView class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "TasksView.h"
#include <net/utf8.hpp>
#include <algorithm>
#include <sstream>
#include <gui/styles.hpp>
#ifdef CAIRO_PAINTER
#include <gui/cairo_painter.hpp>
#include <cairo/cairo-win32.h>
#else
#include <gui/gdi_painter.hpp>
#endif
#include <gui/nodes/doc_element.hpp>
#include <cmath>

#if FA_CHEATSHEET
#include "gui/font_awesome.hh"
#include <gui/nodes/row_node.hpp>
#include <gui/nodes/table_node.hpp>
#include <gui/nodes/text_node.hpp>
#endif

namespace {
	const std::shared_ptr<styles::stylesheet>& stylesheet();
}

gui::ratio zooms[] = {
	{  1,  4 },
	{  1,  3 },
	{  1,  2 },
	{  2,  3 },
	{  9, 10 },
	{  1,  1 }, /* 5 */
	{ 11, 10 },
	{  5,  4 },
	{  3,  2 },
	{  7,  4 },
	{  2,  1 },
	{  3,  1 },
	{  4,  1 },
	{  5,  1 }
};

size_t defaultZoom = 5;

class TaskViewModelListener : public CAppModelListener {
	HWND m_hWnd;
public:
	TaskViewModelListener(HWND hWnd) : m_hWnd(hWnd) {}

	void onListChanged(uint32_t addedOrRemoved) override
	{
		// traversing the borders of threads, if needed
		// also, the actual reaction to the event will
		// not block the emit up there
		if (IsWindow(m_hWnd))
			PostMessage(m_hWnd, UM_LISTCHANGED, addedOrRemoved, 0);
	}
};

class ServerListener : public jira::server_listener {
	HWND m_hWnd;
	uint32_t m_sessionId;
public:
	ServerListener(HWND hWnd, uint32_t sessionId) : m_hWnd(hWnd), m_sessionId(sessionId) {}

	void onRefreshStarted() override
	{
		if (IsWindow(m_hWnd))
			PostMessage(m_hWnd, UM_REFRESHSTART, m_sessionId, 0);
	}

	void onRefreshFinished() override
	{
		if (IsWindow(m_hWnd))
			PostMessage(m_hWnd, UM_REFRESHSTOP, m_sessionId, 0);
	}

	void onProgress(bool calculable, uint64_t content, uint64_t loaded) override
	{
		if (!IsWindow(m_hWnd))
			return;

		ProgressInfo info{ content, loaded, calculable };
		SendMessage(m_hWnd, UM_PROGRESS, m_sessionId, (LPARAM)&info);
	}
};

std::function<void(const gui::point&, const gui::size&)>
	make_invalidator(HWND hWnd, const std::shared_ptr<ZoomInfo>& info)
{
	auto info_ = info;
	return [hWnd, info_](const gui::point& pt, const gui::size& sz) {
		gui::ratio zoom = info_->mul;
		RECT r{
			(LONG)std::floor(zoom.scaleD(pt.x)),
			(LONG)std::floor(zoom.scaleD(pt.y)),
			(LONG)std::ceil(zoom.scaleD(pt.x + sz.width)),
			(LONG)std::ceil(zoom.scaleD(pt.y + sz.height))
		};
#ifdef DEBUG_UPDATES
		info_->updates.push_back(r);
		::InvalidateRect(hWnd, nullptr, TRUE);
#else
		::InvalidateRect(hWnd, &r, TRUE);
#endif
	};
}

CTasksView::ServerInfo::ServerInfo(
	const std::shared_ptr<jira::server>& server,
	const std::shared_ptr<gui::document>& doc,
	const std::shared_ptr<jira::server_listener>& listener)
	: m_server(server)
	, m_document(doc)
	, m_listener(listener)
	, m_sessionId(server->sessionId())
{
	m_server->registerListener(m_listener);
	buildPlaque();
}

CTasksView::ServerInfo::~ServerInfo()
{
	if (m_server && m_listener)
		m_server->unregisterListener(m_listener);
}

void CTasksView::ServerInfo::buildPlaque()
{
	if (m_plaque) {
		auto parent = m_plaque->getParent();
		if (parent)
			parent->removeChild(m_plaque);
	}
	m_plaque = std::make_unique<CJiraReportElement>(m_dataset);
	std::static_pointer_cast<CJiraReportElement>(m_plaque)->addChildren(*m_server, m_document.get());
	m_plaque->setId("server-" + std::to_string(m_server->sessionId()));
}

std::vector<CTasksView::ServerInfo>::iterator CTasksView::find(uint32_t sessionId)
{
	return std::find_if(std::begin(m_servers), std::end(m_servers), [sessionId](const ServerInfo& info) { return info.m_sessionId == sessionId; });
}

std::vector<CTasksView::ServerInfo>::iterator CTasksView::insert(std::vector<ServerInfo>::const_iterator it, const ::ServerInfo& info)
{
	// TODO: create UI element
	// TDOD: attach refresh listener to the server
	auto listener = std::make_shared<ServerListener>(m_hWnd, info.m_server->sessionId());
	return m_servers.emplace(it, info.m_server, info.m_document, listener);
}

std::vector<CTasksView::ServerInfo>::iterator CTasksView::erase(std::vector<ServerInfo>::const_iterator it)
{
	// TODO: destroy UI element
	// TDOD: detach refresh listener from the server
	return m_servers.erase(it);
}

BOOL CTasksView::PreTranslateMessage(MSG* pMsg)
{
	pMsg;
	return FALSE;
}

LRESULT CTasksView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	m_currentZoom = defaultZoom;
	m_zoom->device = { 1, 1 };
	m_zoom->zoom = zooms[m_currentZoom];
	m_zoom->mul = m_zoom->device * m_zoom->zoom;

	m_body = std::make_shared<gui::doc_element>(make_invalidator(m_hWnd, m_zoom));

	RECT empty{ 0, 0, 0, 0 };
	m_tooltip.Create(TOOLTIPS_CLASS, m_hWnd, empty, nullptr, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, WS_EX_TOPMOST);
	m_tooltip.SetWindowPos(HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	m_tooltip.SendMessage(TTM_SETMAXTIPWIDTH, 0, 700);

	updateCursorAndTooltip(true);
	m_background.CreateSolidBrush(0xFFFFFF);
	m_listener = std::make_shared<TaskViewModelListener>(m_hWnd);
	m_model->registerListener(m_listener);

#if FA_CHEATSHEET
	size_t pos = 0;
	constexpr size_t width = 10;
	auto glyphs = std::make_shared<gui::table_node>();
	auto header = std::make_shared<gui::row_node>(gui::elem::table_head);
	glyphs->appendChild(header);
	for (size_t i = 0; i < width; ++i) {
		auto th = std::make_shared<gui::span_node>(gui::elem::th);
		th->innerText("G");
		header->appendChild(th);

		th = std::make_shared<gui::span_node>(gui::elem::th);
		th->innerText("Name");
		header->appendChild(th);
	}

	auto rowCount = ((size_t)fa::glyph::__last_glyph + width - 1) / width;
	for (size_t r = 0; r < rowCount; ++r) {
		auto row = std::make_shared<gui::row_node>(gui::elem::table_row);
		glyphs->appendChild(row);
		for (size_t i = 0; i < width; ++i) {
#ifdef FA_CHEATSHEET_ROW_FIRST
			auto id = r * width + i;
#else
			auto id = i * rowCount + r;
#endif
			if (id >= (size_t)fa::glyph::__last_glyph)
				continue;

			wchar_t s[2] = {};
			s[0] = fa::glyph_char((fa::glyph)id);
			s[1] = 0;
			auto td = std::make_shared<gui::span_node>(gui::elem::td);
			auto g = std::make_shared<gui::text_node>(utf::narrowed(s));
			g->addClass("symbol");
			td->appendChild(g);
			row->appendChild(td);

			td = std::make_shared<gui::span_node>(gui::elem::td);
			auto n = std::make_shared<gui::text_node>(fa::glyph_name((fa::glyph)id));
			n->addClass("summary");
			td->appendChild(n);
			row->appendChild(td);
		}
	}
	m_body->appendChild(glyphs);
#endif
	updateLayout();

	return 0;
}

LRESULT CTasksView::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	m_model->unregisterListener(m_listener);
	return 0;
}

namespace {

	std::shared_ptr<styles::stylesheet> stylesheetCreate()
	{
		using namespace gui::literals;
		using namespace styles::literals;
		using namespace styles::def;
		using namespace styles;

		auto none_empty = italic() << color(0x555555);

		styles::stylesheet out;

		// "Default" stylesheet
		out
			// GENERIC ELEMENTS
			.add(gui::elem::body,                          display(gui::display::block) << padding(7_px))
			.add(gui::elem::block,                         display(gui::display::block))
			.add(gui::elem::header,                        display(gui::display::block) <<
			                                               font_size(1.8_em) <<
			                                               padding(.5_em, .2_px, .4_em))
			                                               // margin(.3_em, 0_px, .2_em) <<
			                                               // padding(.2_em, .2_px, .2_em))

			// TABLE
			.add(gui::elem::table,                         display(gui::display::table))
			.add(gui::elem::table_caption,                 display(gui::display::table_caption) <<
			                                               padding(.2_em))
			.add(gui::elem::table_head,                    display(gui::display::table_header))
			.add(gui::elem::table_row,                     display(gui::display::table_row))
			.add(gui::elem::td,                            display(gui::display::table_cell) <<
			                                               padding(.2_em))
			.add(gui::elem::th,                            display(gui::display::table_cell) <<
			                                               text_align(gui::align::center) <<
			                                               font_weight(gui::weight::bold) <<
			                                               padding(.2_em))

			// LINK
			.add(gui::elem::link,                          color(0xAF733B) << cursor(gui::pointer::hand))
			.add({ gui::elem::link, pseudo::hover },       underline());

		// Application styleshet
		out
			.add(gui::elem::header,                        color(0x883333))
			.add(gui::elem::table,                         border(1_px, gui::line_style::solid, 0xc0c0c0))
			.add(gui::elem::table_row,                     border_top(1_px, gui::line_style::solid, 0xc0c0c0))
			.add({ gui::elem::table_row, pseudo::hover },  background(0xf5f5f5))
			.add(gui::elem::table_caption,                 background(0xaf733b) << color(0xFFFFFF) <<
			                                               font_weight(gui::weight::bold) <<
			                                               font_size(.75_em) << padding(.6_em) <<
			                                               font_family("Courier New"))
			.add(gui::elem::td,                            padding(.2_em, .6_em))
			.add(gui::elem::th,                            padding(.2_em, .6_em))
			.add(gui::elem::link,                          padding(2_px) << border(1_px, gui::line_style::none, 0xc0c0c0))
			.add({ gui::elem::link, pseudo::active },      border(1_px, gui::line_style::dot, 0xc0c0c0))

			.add(class_name{ "error" },                    color(0x171BC1))
			.add(class_name{ "empty" },                    none_empty)
			.add(class_name{ "none" },                     none_empty)
			.add(class_name{ "summary" },                  font_size(.8_em) << color(0x555555))
			.add(class_name{ "symbol" },                   font_family("FontAwesome"))
			.add(class_name{ "unexpected" },               color(0x2600E6))
			.add(class_name{ "label" },                    background(0xF5F5F5) <<
			                                               border(1_px, gui::line_style::solid, 0xCCCCCC) <<
			                                               padding(1_px, 5_px))
			.add(class_name{ "parent-link" },              color(0x666666));

		return std::make_shared<styles::stylesheet>(std::move(out));
	};

	const std::shared_ptr<styles::stylesheet>& stylesheet()
	{
		static std::shared_ptr<styles::stylesheet> sheet = stylesheetCreate();
		return sheet;
	}
};

LRESULT CTasksView::OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CPaintDC dc(m_hWnd);

	m_zoom->device = gui::ratio{ dc.GetDeviceCaps(LOGPIXELSX), 96 }.gcd();
	m_zoom->mul = m_zoom->device * m_zoom->zoom;
#ifdef CAIRO_PAINTER
	gui::cairo::painter paint{ cairo_win32_surface_create(dc), m_zoom->mul, m_fontSize, m_fontFamily };
#else
	gui::gdi::painter paint{ (HDC)dc, (HBRUSH)m_background, m_zoom->mul, dc.m_ps.rcPaint, m_fontSize, m_fontFamily };
#endif

	m_body->paint(&paint);

#ifdef DEBUG_UPDATES
	std::vector<RECT> updates;
	std::swap(updates, m_zoom->updates);

	for (auto& r : updates) {
		COLORREF clr = 0x66cc66;
		CDCHandle{ paint.dc() }.Draw3dRect(&r, clr, clr);
	}
#endif

	return 0;
}

std::shared_ptr<gui::node> nextSibling(const std::shared_ptr<gui::node>& parent, const std::shared_ptr<gui::node>& prev)
{
	auto& children = parent->children();
	if (!prev)
		return children.empty() ? nullptr : children[0];

	auto it = std::find(std::begin(children), std::end(children), prev);
	if (it != std::end(children))
		++it;
	if (it == std::end(children))
		return{};

	return *it;
}

void CTasksView::updateLayout()
{
	ATLASSERT(m_body);

	std::shared_ptr<gui::node> prev;
	for (auto& server : m_servers) {
		ATLASSERT(server.m_plaque);
		auto next = nextSibling(m_body, prev);
		if (next != server.m_plaque)
			m_body->insertBefore(server.m_plaque, next);
		prev = server.m_plaque;
	}

	m_body->applyStyles(stylesheet());

	CWindowDC dc{m_hWnd};


	m_zoom->device = gui::ratio{ dc.GetDeviceCaps(LOGPIXELSX), 96 }.gcd();
	m_zoom->mul = m_zoom->device * m_zoom->zoom;
#ifdef CAIRO_PAINTER
	gui::cairo::painter paint{ cairo_win32_surface_create(dc), m_zoom->mul, m_fontSize, m_fontFamily };
#else
	gui::gdi::painter paint{ (HDC)dc, nullptr, m_zoom->mul, m_fontSize, m_fontFamily };
#endif

	if (m_hovered)
		m_hovered->setHovered(false);
	m_hovered = nullptr;

	if (m_active)
		m_active->setActive(false);
	m_active = nullptr;

	m_body->measure(&paint);
	setDocumentSize(m_body->getSize());

	m_hovered = nodeFromPoint();
	if (m_hovered)
		m_hovered->setHovered(true);
	updateCursorAndTooltip();
}

void CTasksView::updateCursor(bool force)
{
	auto tmp = gui::pointer::arrow;
	if (m_hovered)
		tmp = m_hovered->getCursor();

	if (tmp == gui::pointer::inherited)
		tmp = gui::pointer::arrow;

	if (tmp == m_cursor && !force)
		return;

	m_cursor = tmp;
	LPCWSTR idc = IDC_ARROW;
	switch (m_cursor) {
	case gui::pointer::hand:
		idc = IDC_HAND;
		break;
	};

	if (!m_cursorObj.IsNull())
		m_cursorObj.DestroyCursor();
	m_cursorObj.LoadSysCursor(idc);
	SetCursor(m_cursorObj);
}

void CTasksView::updateTooltip(bool /*force*/)
{
	RECT tool{ 0, 0, 0, 0 };
	std::string tooltip;
	auto node = m_hovered;
	while (node) {
		if (node->hasTooltip()) {
			auto zoom = m_zoom->mul;
			auto pt = m_hovered->getAbsolutePos();
			auto sz = m_hovered->getSize();
			RECT r{
				zoom.scaleL(pt.x) - 2,
				zoom.scaleL(pt.y) - 2,
				zoom.scaleL(pt.x + sz.width) + 2 ,
				zoom.scaleL(pt.y + sz.height) + 2
			};
			tool = r;
			tooltip = node->getTooltip();
			break;
		}

		node = node->getParent();
	}

	{
		TOOLINFO ti = { 0 };
		ti.cbSize = sizeof(TOOLINFO);
		ti.hwnd = m_hWnd;
		ti.hinst = _Module.GetModuleInstance();

		m_tooltip.SendMessage(TTM_DELTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
	}
	if (!tooltip.empty()) {
		auto tmp = utf::widen(tooltip);
		decltype(tmp) widen;
		widen.reserve(tmp.length() * 11 / 10);

		for (auto c : tmp) {
			if (c == L'\n')
				widen += L"\r\n";
			else widen.push_back(c);
		}

		TOOLINFO ti = { 0 };
		ti.cbSize = sizeof(TOOLINFO);
		ti.uFlags = TTF_SUBCLASS;
		ti.hwnd = m_hWnd;
		ti.hinst = _Module.GetModuleInstance();
		ti.lpszText = (LPWSTR)widen.c_str();
		ti.rect = tool;

		m_tooltip.SendMessage(TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
	}
}

#ifdef DEBUG_STYLES
static std::string to_string(gui::elem name)
{
	using namespace gui;
	switch (name) {
	case elem::unspecified: return{}; break;
	case elem::body: return "body"; break;
	case elem::block: return "div"; break;
	case elem::header: return "h1"; break;
	case elem::span: return "span"; break;
	case elem::text: return "TXT"; break;
	case elem::link: return "a"; break;
	case elem::image: return "img"; break;
	case elem::icon: return "icon"; break;
	case elem::table: return "table"; break;
	case elem::table_head: return "t-head"; break;
	case elem::table_row: return "t-row"; break;
	case elem::th: return "th"; break;
	case elem::td: return "td"; break;
	};

	return "{" + std::to_string((int)name) + "}";
}

static std::string to_string(const styles::selector& sel)
{
	std::string out = to_string(sel.m_elemName);

	for (auto& cl : sel.m_classes) {
		out.push_back('.');
		out.append(cl);
	}

	using namespace styles;
	switch (sel.m_pseudoClass) {
	case pseudo::hover: out += ":hover"; break;
	case pseudo::active: out += ":active"; break;
	};

	if (out.empty())
		out = "*";
	return out;
}

static std::string to_string(styles::color_prop prop) {
	switch(prop) {
	case styles::prop_color: return "color";
	case styles::prop_background: return "background";
	case styles::prop_border_top_color: return "border-top-color";
	case styles::prop_border_right_color: return "border-right-color";
	case styles::prop_border_bottom_color: return "border-bottom-color";
	case styles::prop_border_left_color: return "border-left-color";
	};

	return "{" + std::to_string((int)prop) + "}";
};

static std::string to_string(styles::bool_prop prop) {
	switch(prop) {
	case styles::prop_italic: return "italic";
	case styles::prop_underline: return "underline";
	}

	return "{" + std::to_string((int)prop) + "}";
};

static std::string to_string(styles::string_prop prop) {
	switch(prop) {
	case styles::prop_font_family: return "font-family";
	}

	return "{" + std::to_string((int)prop) + "}";
};

static std::string to_string(styles::length_prop prop) {
	switch(prop) {
	case styles::prop_border_top_width: return "border-top-width";
	case styles::prop_border_right_width: return "border-right-width";
	case styles::prop_border_bottom_width: return "border-bottom-width";
	case styles::prop_border_left_width: return "border-left-width";
	case styles::prop_font_size: return "font-size";
	case styles::prop_padding_top: return "padding-top";
	case styles::prop_padding_right: return "padding-right";
	case styles::prop_padding_bottom: return "padding-bottom";
	case styles::prop_padding_left: return "padding-left";
	case styles::prop_margin_top: return "margin-top";
	case styles::prop_margin_right: return "margin-right";
	case styles::prop_margin_bottom: return "margin-bottom";
	case styles::prop_margin_left: return "margin-left";
	}

	return "{" + std::to_string((int)prop) + "}";
};

static std::string to_string(styles::font_weight_prop /*prop*/) { return "font-weight"; }
static std::string to_string(styles::text_align_prop /*prop*/) { return "text-align"; }
static std::string to_string(styles::display_prop /*prop*/) { return "display"; }

static std::string to_string(styles::border_style_prop prop) {
	switch (prop) {
	case styles::prop_border_top_style: return "border-top-style";
	case styles::prop_border_right_style: return "border-right-style";
	case styles::prop_border_bottom_style: return "border-bottom-style";
	case styles::prop_border_left_style: return "border-left-style";
	};

	return "{" + std::to_string((int)prop) + "}";
}

static std::string to_string(gui::colorref col)
{
	char buffer[100];
	sprintf_s(buffer, "#%06X", col);
	return buffer;
};

static std::string to_string(bool val)
{
	return val ? "true" : "false";
};

static const std::string& to_string(const std::string& val)
{
	return val;
};

static std::string to_string(const gui::pixels& val)
{
	return std::to_string(val.value()) + "px";
};

static std::string to_string(const styles::ems& val)
{
	return std::to_string(val.value()) + "em";
};

static std::string to_string(const styles::length_u& len)
{
	if (len.which() == styles::length_u::first_type) {
		return to_string(len.first());
	} else if (len.which() == styles::length_u::second_type) {
		return to_string(len.second());
	} else {
		return "nullptr";
	}
};

static std::string to_string(gui::weight val)
{
	switch (val) {
	case gui::weight::normal: return "normal";
	case gui::weight::bold: return "bold";
	case gui::weight::bolder: return "bolder";
	case gui::weight::lighter: return "lighter";
	case gui::weight::w100: return "100";
	case gui::weight::w200: return "200";
	case gui::weight::w300: return "300";
	case gui::weight::w400: return "400";
	case gui::weight::w500: return "500";
	case gui::weight::w600: return "600";
	case gui::weight::w700: return "700";
	case gui::weight::w800: return "800";
	case gui::weight::w900: return "900";
	}

	return std::to_string((int)val);
}

static std::string to_string(gui::align val)
{
	switch (val) {
	case gui::align::left: return "left";
	case gui::align::right: return "right";
	case gui::align::center: return "center";
	}

	return std::to_string((int)val);
}

static std::string to_string(gui::line_style val)
{
	switch (val) {
	case gui::line_style::none: return "none";
	case gui::line_style::solid: return "solid";
	case gui::line_style::dot: return "dat";
	case gui::line_style::dash: return "dash";
	}

	return std::to_string((int)val);
}

static std::string to_string(gui::display val)
{
	switch (val) {
	case gui::display::inlined: return "inline";
	case gui::display::block: return "block";
	case gui::display::table: return "table";
	case gui::display::table_row: return "table-row";
	case gui::display::table_caption: return "table-caption";
	case gui::display::table_header: return "table-header-row";
	case gui::display::table_footer: return "table-footer-row";
	case gui::display::table_cell: return "table-cell";
	case gui::display::none: return "none";
	}

	return std::to_string((int)val);
}

template <typename Prop>
static void debug_rule(std::vector<std::pair<std::string, std::string>>& values, Prop prop, const styles::rule_storage* rules) {
	if (!rules->has(prop))
		return;

	values.emplace_back(to_string(prop), to_string(rules->get(prop)));
}

static void debug_rules(const styles::rule_storage* rules) {
	std::vector<std::pair<std::string, std::string>> values;
	debug_rule(values, styles::prop_color, rules);
	debug_rule(values, styles::prop_background, rules);
	debug_rule(values, styles::prop_border_top_color, rules);
	debug_rule(values, styles::prop_border_right_color, rules);
	debug_rule(values, styles::prop_border_bottom_color, rules);
	debug_rule(values, styles::prop_border_left_color, rules);
	debug_rule(values, styles::prop_display, rules);
	debug_rule(values, styles::prop_italic, rules);
	debug_rule(values, styles::prop_underline, rules);
	debug_rule(values, styles::prop_visibility, rules);
	debug_rule(values, styles::prop_font_family, rules);
	debug_rule(values, styles::prop_font_size, rules);
	debug_rule(values, styles::prop_border_top_width, rules);
	debug_rule(values, styles::prop_border_right_width, rules);
	debug_rule(values, styles::prop_border_bottom_width, rules);
	debug_rule(values, styles::prop_border_left_width, rules);
	debug_rule(values, styles::prop_padding_top, rules);
	debug_rule(values, styles::prop_padding_right, rules);
	debug_rule(values, styles::prop_padding_bottom, rules);
	debug_rule(values, styles::prop_padding_left, rules);
	debug_rule(values, styles::prop_margin_top, rules);
	debug_rule(values, styles::prop_margin_right, rules);
	debug_rule(values, styles::prop_margin_bottom, rules);
	debug_rule(values, styles::prop_margin_left, rules);
	debug_rule(values, styles::prop_font_weight, rules);
	debug_rule(values, styles::prop_text_align, rules);
	debug_rule(values, styles::prop_border_top_style, rules);
	debug_rule(values, styles::prop_border_right_style, rules);
	debug_rule(values, styles::prop_border_bottom_style, rules);
	debug_rule(values, styles::prop_border_left_style, rules);

	std::sort(std::begin(values), std::end(values));

	for (auto& pair : values)
		OutputDebugStringA(("   " + pair.first + ": " + pair.second + ";\n").c_str());
};
#endif // DEBUG_STYLES

void CTasksView::updateCursorAndTooltip(bool force)
{
	updateCursor(force);
	updateTooltip(force);

#ifdef DEBUG_STYLES
	if (m_hovered) {
		OutputDebugString(L"======================================================\n");
		auto node = m_hovered;
		while (node) {
			{
				std::string klass = "(";
				klass.append(to_string(node->getNodeName()));
				for (auto& kl : node->getClassNames()) {
					klass.push_back('.');
					klass.append(kl);
				}
				klass += ")\n";
				OutputDebugStringA(klass.c_str());
			}
			auto styles = node->styles();
			if (!styles) {
				OutputDebugString(L"!!!\n");
			} else {
				for (auto& rule : styles->m_rules) {
					OutputDebugStringA((to_string(rule->m_sel) + " {\n").c_str());
					debug_rules(rule.get());
					OutputDebugString(L"}\n");
				}
			}
			node = node->getParent();
			if (node)
				OutputDebugString(L"------------------------------------------------------\n");
		}
		OutputDebugString(L"######################################################\n");
		node = m_hovered;
		while (node) {
			{
				std::string klass = "(";
				klass.append(to_string(node->getNodeName()));
				for (auto& kl : node->getClassNames()) {
					klass.push_back('.');
					klass.append(kl);
				}
				klass += ")\n";
				OutputDebugStringA(klass.c_str());
			}
			auto styles = node->calculatedStyle();
			if (!styles) {
				OutputDebugString(L"!!!\n");
			} else {
				debug_rules(styles.get());
			}
			node = node->getParent();
			if (node)
				OutputDebugString(L"------------------------------------------------------\n");
		}
		OutputDebugString(L"======================================================\n");
	}
#endif // DEBUG_STYLES
}

std::shared_ptr<gui::node> CTasksView::nodeFromPoint()
{
	return m_body->nodeFromPoint(m_mouse.x, m_mouse.y);
}

void CTasksView::setDocumentSize(const gui::size& newSize)
{
	m_docSize = newSize;
	updateDocumentSize();
}

void CTasksView::updateDocumentSize()
{
	if (m_scroller)
		m_scroller(m_zoom->mul.scaleL(m_docSize.width), m_zoom->mul.scaleL(m_docSize.height));
}

void CTasksView::mouseFromMessage(LPARAM lParam)
{
	auto mouseX = GET_X_LPARAM(lParam);
	auto mouseY = GET_Y_LPARAM(lParam);
	m_mouse = { m_zoom->mul.invert(mouseX), m_zoom->mul.invert(mouseY) };
}

void CTasksView::zoomIn()
{
	if (m_currentZoom < (sizeof(zooms)/sizeof(zooms[0]) - 1))
		setZoom(m_currentZoom + 1);
}

void CTasksView::zoomOut()
{
	if (m_currentZoom > 0)
		setZoom(m_currentZoom - 1);
}

void CTasksView::setZoom(size_t newLevel)
{
	m_currentZoom = newLevel;
	m_zoom->zoom = zooms[m_currentZoom];
	m_zoom->mul = m_zoom->device * m_zoom->zoom;
	Invalidate();
	updateDocumentSize();

	auto tmp = nodeFromPoint();
	if (tmp != m_hovered) {
		if (tmp)
			tmp->setHovered(true);
		if (m_hovered)
			m_hovered->setHovered(false);

		m_hovered = tmp;
		updateCursorAndTooltip();
	}
}

LRESULT CTasksView::OnSetFont(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
	bHandled = FALSE;
	m_font = (HFONT) wParam;

	LOGFONT lf = {};
	m_font.GetLogFont(lf);

	{
		CWindowDC dc{m_hWnd};
		m_fontSize = -lf.lfHeight * 96.0 / dc.GetDeviceCaps(LOGPIXELSY);
	}

	m_fontFamily = utf::narrowed(lf.lfFaceName);


	updateLayout();
	// TODO: redraw the report table

	return 0;
}

LRESULT CTasksView::OnMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
	mouseFromMessage(lParam);

	auto tmp = nodeFromPoint();
	if (tmp != m_hovered) {
		if (tmp)
			tmp->setHovered(true);
		if (m_hovered)
			m_hovered->setHovered(false);

		m_hovered = tmp;
		updateCursorAndTooltip();
	}
	return 0;
}

LRESULT CTasksView::OnMouseDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
	mouseFromMessage(lParam);

	auto tmp = m_active;
	m_active = nodeFromPoint();
	if (m_active)
		m_active->setActive(true);
	if (tmp)
		tmp->setActive(false);

	m_tracking = true;
	SetCapture();
	Invalidate();

	return 0;
}

LRESULT CTasksView::OnMouseUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
	mouseFromMessage(lParam);

	auto tmp = nodeFromPoint();
	m_tracking = false;
	ReleaseCapture();
	Invalidate();

	if (tmp && tmp == m_active)
		m_active->activate();

	return 0;
}

LRESULT CTasksView::OnSetCursor(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
{
	bHandled = LOWORD(lParam) == HTCLIENT;
	if (bHandled) {
		SetCursor(m_cursorObj);
	}

	return bHandled;
}

LRESULT CTasksView::OnListChanged(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// TODO: lock updates
	if (wParam) { 
		// a server has been added or removed...
		auto it = find(wParam);
		if (it == m_servers.end()) { // it was added
			synchronize(*m_model, [&] {
				auto& servers = m_model->servers();
				// find the location of the ID; hint: it might be at the end
				auto sit = std::find_if(std::begin(servers), std::end(servers), [&](const ::ServerInfo& info) { return info.m_server->sessionId() == wParam; });
				if (sit != std::end(servers)) {
					auto it = std::begin(m_servers);

					// it += (rit.base() - servers.begin())
					 std::advance(it, std::distance(std::begin(servers), sit));

					 insert(it, *sit);
				}
			});
		} else
			erase(it);
	} else {
		// no specific item has changed, go over entire model
		synchronize(*m_model, [&] {
			auto& servers = m_model->servers();

			auto it = std::begin(m_servers);
			auto end = std::end(m_servers);

			// for each ServerInfo: if not in model - remove
			while (it != end) {
				auto query = std::find_if(std::begin(servers), std::end(servers), [&](const ::ServerInfo& info) { return info.m_server->sessionId() == it->m_sessionId; });
				if (query == std::end(servers))
				{
					it = erase(it);
					end = std::end(m_servers);
				}
			}

			// for each server in model: if not in ServerInfos - add
			// side note: we have only items from the model, granted maybe not the ALL items
			// if we kept track of additions/deletions, they are in the same order as in the model
			// we just need to fill in the holes...

			it = std::begin(m_servers);
			end = std::end(m_servers);
			auto srv_it = std::begin(servers);
			auto srv_end = std::end(servers);

			for (; srv_it != srv_end; ++srv_it, ++it) {
				if (it != end && (srv_it->m_server)->sessionId() == it->m_sessionId)
					continue;

				it = insert(it, *srv_it); // fill in blanks...
				end = std::end(m_servers);
			}
		});
	}
	updateLayout();
	// TODO: unlock updates
	Invalidate();

	return 0;
}

LRESULT CTasksView::OnRefreshStart(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	auto it = find(wParam);
	if (it == m_servers.end())
		return 0;

	it->m_progress = ProgressInfo{100, 0, true};
	it->m_loading = true;
	Invalidate();

	return 0;
}

LRESULT CTasksView::OnRefreshStop(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	auto it = find(wParam);
	if (it == m_servers.end())
		return 0;

	it->m_loading = false;
	it->m_gotProgress = false;

	auto& server = *it->m_server;
	it->m_dataset = server.dataset();
	it->buildPlaque();
	updateLayout();
	// TODO: redraw the report table
	Invalidate();

	return 0;
}

LRESULT CTasksView::OnProgress(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
	if (!lParam)
		return 0;

	auto it = find(wParam);
	if (it == m_servers.end())
		return 0;

	it->m_progress = *reinterpret_cast<ProgressInfo*>(lParam);
	it->m_gotProgress = true;
	Invalidate();

	return 0;
}

LRESULT CTasksView::OnZoom(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
	int lines = (int)wParam;
	if (lParam){
		while (lines--)
			zoomOut();
	} else {
		while (lines--)
			zoomIn();
	}

	return 0;
}
