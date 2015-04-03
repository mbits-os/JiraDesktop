// TasksView.cpp : implementation of the CTasksView class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "TasksView.h"
#include <net/utf8.hpp>
#include <algorithm>
#include <sstream>

#include "AppNodes.h"

#if FA_CHEATSHEET
#include "font_awesome.hh"
#endif

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

std::function<void(int, int, int, int)> make_invalidator(HWND hWnd) {
	return [hWnd](int x, int y, size_t width, size_t height) {
		RECT r{ x, y, x + (int)width, y + (int)height };
		::InvalidateRect(hWnd, &r, TRUE);
	};
}

CTasksView::ServerInfo::ServerInfo(const std::shared_ptr<jira::server>& server, const std::shared_ptr<jira::server_listener>& listener, HWND hWnd)
	: m_server(server)
	, m_listener(listener)
	, m_sessionId(server->sessionId())
{
	m_server->registerListener(m_listener);
	m_plaque = std::make_unique<CJiraReportElement>(m_dataset, make_invalidator(hWnd));
	std::static_pointer_cast<CJiraReportElement>(m_plaque)->addChildren(*server);
}

CTasksView::ServerInfo::~ServerInfo()
{
	if (m_server && m_listener)
		m_server->unregisterListener(m_listener);
}

std::vector<CTasksView::ServerInfo>::iterator CTasksView::find(uint32_t sessionId)
{
	return std::find_if(std::begin(m_servers), std::end(m_servers), [sessionId](const ServerInfo& info) { return info.m_sessionId == sessionId; });
}

std::vector<CTasksView::ServerInfo>::iterator CTasksView::insert(std::vector<ServerInfo>::const_iterator it, const std::shared_ptr<jira::server>& server)
{
	// TODO: create UI element
	// TDOD: attach refresh listener to the server
	auto listener = std::make_shared<ServerListener>(m_hWnd, server->sessionId());
	return m_servers.emplace(it, server, listener, m_hWnd);
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
	RECT empty{ 0, 0, 0, 0 };
	m_tooltip.Create(TOOLTIPS_CLASS, m_hWnd, empty, nullptr, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, WS_EX_TOPMOST);
	m_tooltip.SetWindowPos(HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	m_tooltip.SendMessage(TTM_SETMAXTIPWIDTH, 0, 700);

	updateCursorAndTooltip(true);
	m_background.CreateSolidBrush(0x00FFFFFF);
	m_listener = std::make_shared<TaskViewModelListener>(m_hWnd);
	m_model->registerListener(m_listener);

#if FA_CHEATSHEET
	size_t pos = 0;
	constexpr size_t width = 10;
	auto glyphs = std::make_shared<CJiraTableNode>();
	auto header = glyphs->addHeader();
	for (size_t i = 0; i < width; ++i) {
		header->addChild(std::make_shared<CJiraTextNode>("G"));
		header->addChild(std::make_shared<CJiraTextNode>("Name"));
	}

	auto rowCount = ((size_t)fa::glyph::__last_glyph + width - 1) / width;
	for (size_t r = 0; r < rowCount; ++r) {
		auto row = glyphs->addRow();
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
			auto g = std::make_shared<CJiraTextNode>(utf::narrowed(s));
			g->setClass(rules::symbol);
			row->addChild(g);
			auto n = std::make_shared<CJiraTextNode>(fa::glyph_name((fa::glyph)id));
			n->setClass(rules::classSummary);
			row->addChild(n);
		}
	}
	m_cheatsheet = glyphs;
#endif
	return 0;
}

LRESULT CTasksView::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	m_model->unregisterListener(m_listener);
	return 0;
}

enum {
	BODY_MARGIN = 7
};

namespace { class Styler; };

enum class bk {
	transparent,
	solid
};

class LinePrinter : public IJiraPainter
{
	HFONT older;
	CFontHandle font;
	CDCHandle dc;
	int x = BODY_MARGIN;
	int y = BODY_MARGIN;
	Styler* uplink = nullptr;
	bool selectedFrame = false;
	bk backgroundMode = bk::transparent;
	COLORREF color = 0x00FFFFFF;
	RECT update;

	void updateLineHeight()
	{
		TEXTMETRIC metric = {};
		dc.GetTextMetrics(&metric);
	}

	void moveOrigin(int x_, int y_) override
	{
		x += x_;
		y += y_;
	}

	point getOrigin() const override
	{
		return{ x, y };
	}

	void setOrigin(const point& orig) override
	{
		x = orig.x;
		y = orig.y;
	}

	void paintImage(const std::string& /*url*/, size_t width, size_t height) override
	{
		// TODO: get url from pixmap cache
		dc.Rectangle(x, y, x + width, y + height);
	}

	void paintImage(const ImageRef* img, size_t width, size_t height) override
	{
		auto bmp = reinterpret_cast<Gdiplus::Bitmap*>(img ? img->getNativeHandle() : nullptr);
		if (img && img->getState() != load_state::pixmap_available)
			bmp = nullptr;

		if (!bmp) {
			dc.Rectangle(x, y, x + width, y + height);
			return;
		}

		Gdiplus::Graphics gfx{ (HDC)dc };
		gfx.DrawImage(bmp, x, y, width, height);
	}

	void paintString(const std::string& text) override
	{
		if (text.empty())
			return;

		auto widen = utf::widen(text);
		dc.TextOut(x, y, widen.c_str());

#if 0
		SIZE s = {};
		TEXTMETRIC tm = {};
		dc.GetTextExtent(widen.c_str(), widen.length(), &s);
		dc.GetTextMetrics(&tm);
		dc.FillSolidRect(x, y + tm.tmAscent, s.cx - 1, 1, 0x003333FF);
#endif
	}

	size measureString(const std::string& text) override
	{
		auto line = utf::widen(text);
		SIZE s;
		dc.GetTextExtent(line.c_str(), line.length(), &s);
		return{ (size_t)s.cx, (size_t)s.cy };
	}

	StyleSave* setStyle(jira::styles, IJiraNode*) override;
	StyleSave* setStyle(rules, IJiraNode*) override;
	void restoreStyle(StyleSave* save) override;
	int dpiRescale(int size) override;
public:
	explicit LinePrinter(HDC dc_, HFONT font_) : dc(dc_), font(font_)
	{
		older = dc.SelectFont(font);
		updateLineHeight();
	}
	~LinePrinter()
	{
		dc.SelectFont(older);
	}

	LinePrinter& select(HFONT font_)
	{
		font = font_;
		dc.SelectFont(font);
		updateLineHeight();
		return *this;
	}

	LinePrinter& drawFocus(jira::node* node)
	{
		if (selectedFrame) {
			selectedFrame = false;
			auto pt = static_cast<IJiraNode*>(node)->getAbsolutePos();
			auto sz = static_cast<IJiraNode*>(node)->getSize();
			RECT r{ pt.x - 2, pt.y - 2, pt.x + (int)sz.width + 2, pt.y + (int)sz.height + 2 };
			dc.DrawFocusRect(&r);
		}
		return *this;
	}

	LinePrinter& drawBackground(jira::node* node)
	{
		if (backgroundMode == bk::solid) {
			backgroundMode = bk::transparent;
			auto pt = static_cast<IJiraNode*>(node)->getAbsolutePos();
			auto sz = static_cast<IJiraNode*>(node)->getSize();
			RECT r{ pt.x - 1, pt.y - 1, pt.x + (int)sz.width + 1, pt.y + (int)sz.height + 1 };
			dc.FillSolidRect(&r, color);
		}

		return *this;
	}

	LinePrinter& paint(const std::shared_ptr<jira::node>& node, Styler* link)
	{
		uplink = link;
		auto size = cast(node)->getSize();

		RECT r = { x, y, (int)size.width + x, (int)size.height + y };
		RECT test;
		IntersectRect(&test, &r, &update);

		if (test.left == test.right || test.top == test.bottom)
			return *this;

		cast(node)->paint(this);
		return *this;
	}

	LinePrinter& measure(const std::shared_ptr<jira::node>& node, Styler* link)
	{
		uplink = link;
		cast(node)->measure(this);
		return *this;
	}

	LinePrinter& setFrameSelect()
	{
		selectedFrame = true;
		return *this;
	}

	LinePrinter& setBackground(bk mode, COLORREF clr)
	{
		backgroundMode = mode;
		color = clr;
		return *this;
	}

	LinePrinter& updateRect(const RECT& val)
	{
		update = val;
		return *this;
	}
};

namespace {

	enum class family {
		base,
		symbols
	};

	class Styler {
		LinePrinter printer;
		CDCHandle dc;
		CFont font;
		LOGFONT logFont;
		COLORREF lastColor;
		family lastFamily;
		std::wstring baseName;

		void update()
		{
			if (font)
				font.DeleteObject();
			font.CreateFontIndirect(&logFont);
			printer.select(font);
		}
	public:
		Styler(HDC dc_, HFONT font_)
			: printer(dc_, font_)
			, dc(dc_)
			, lastColor(0)
			, lastFamily(family::base)
		{
			CFontHandle{ font_ }.GetLogFont(&logFont);
			lastColor = dc.GetTextColor();
			baseName = logFont.lfFaceName;
			update();
		}

		void setColor(COLORREF color)
		{
			lastColor = color;
			dc.SetTextColor(color);
		}

		void setFontItalic(bool italic)
		{
			logFont.lfItalic = italic ? TRUE : FALSE;
			update();
		}

		void setFontUnderline(bool underline)
		{
			logFont.lfUnderline = underline ? TRUE : FALSE;
			update();
		}

		void setFontWeight(int weight)
		{
			logFont.lfWeight = weight;
			update();
		}

		void setFontSize(int size)
		{
			logFont.lfHeight = size;
			update();
		}

		void setFontFamily(family f)
		{
			switch (f) {
			case family::base:
				wcscpy(logFont.lfFaceName, baseName.c_str());
				lastFamily = f;
				break;
			case family::symbols:
				wcscpy(logFont.lfFaceName, L"FontAwesome");
				lastFamily = f;
				break;
			default:
				return;
			}
			update();
		}

		void setFrameSelect()
		{
			printer.setFrameSelect();
		}

		void setBackground(bk mode, COLORREF color)
		{
			printer.setBackground(mode, color);
		}

		COLORREF getColor() const { return lastColor; }
		bool getFontItalic() const { return !!logFont.lfItalic; }
		bool getFontUnderline() const { return !!logFont.lfUnderline; }
		int getFontWeight() const { return logFont.lfWeight; }
		int getFontSize() const { return logFont.lfHeight; }
		family getFontFamily() const { return lastFamily; }
		LinePrinter& out() { return printer; }

		const LOGFONT& fontDef() const { return this->logFont; }
	};

	class Style {
		Styler& m_styler;
		COLORREF m_color;
		int m_weight;
		int m_size;
		bool m_italic;
		bool m_underline;
		family m_family;

	public:
		Style() = delete;
		Style(const Style&) = delete;
		Style& operator=(const Style&) = delete;

		explicit Style(Styler& styler)
			: m_styler(styler)
			, m_color(styler.getColor())
			, m_weight(styler.getFontWeight())
			, m_size(styler.getFontSize())
			, m_italic(styler.getFontItalic())
			, m_underline(styler.getFontUnderline())
			, m_family(styler.getFontFamily())
		{
		}

		explicit Style(Styler& styler, rules rule, IJiraNode* node)
			: m_styler(styler)
			, m_color(styler.getColor())
			, m_weight(styler.getFontWeight())
			, m_size(styler.getFontSize())
			, m_italic(styler.getFontItalic())
			, m_underline(styler.getFontUnderline())
			, m_family(styler.getFontFamily())
		{
			apply(*this, rule, node);
		}

		~Style()
		{
			setColor(m_color);
			setFontWeight(m_weight);
			setFontSize(m_size);
			setFontItalic(m_italic);
			setFontUnderline(m_underline);
			setFontFamily(m_family);
		}

		static void apply(Style& style, rules rule, IJiraNode* node);

		Style& setColor(COLORREF color)
		{
			if (color != m_styler.getColor())
				m_styler.setColor(color);
			return *this;
		}

		Style& setFontItalic(bool italic)
		{
			if (italic != m_styler.getFontItalic())
				m_styler.setFontItalic(italic);
			return *this;
		}

		Style& setFontUnderline(bool underline)
		{
			if (underline != m_styler.getFontUnderline())
				m_styler.setFontUnderline(underline);
			return *this;
		}

		Style& setFontWeight(int weight)
		{
			if (weight != m_styler.getFontWeight())
				m_styler.setFontWeight(weight);
			return *this;
		}

		Style& setFontSize(int size)
		{
			if (size != m_styler.getFontSize())
				m_styler.setFontSize(size);
			return *this;
		}

		Style& setFontFamily(family f)
		{
			if (f != m_styler.getFontFamily())
				m_styler.setFontFamily(f);
			return *this;
		}

		Style& setFrameSelect()
		{
			m_styler.setFrameSelect();
			return *this;
		}

		Style& setBackground(bk mode, COLORREF color)
		{
			m_styler.setBackground(mode, color);
			return *this;
		}
	};

	template <typename T>
	class ManipBase {
	protected:
		T m_data;
	public:
		ManipBase(T data) : m_data(data) {}
	};

	class ColorManip : public ManipBase<COLORREF> {
	public:
		ColorManip(COLORREF color) : ManipBase<COLORREF>(color) {}
		friend Style& operator<<(Style& o, const ColorManip& manip)
		{
			return o.setColor(manip.m_data);
		}
	};

	ColorManip color(COLORREF color) { return ColorManip{ color }; }

	class FontSizeManip : public ManipBase<int> {
	public:
		FontSizeManip(int size) : ManipBase<int>(size) {}
		friend Style& operator<<(Style& o, const FontSizeManip& manip)
		{
			return o.setFontSize(manip.m_data);
		}
	};

	FontSizeManip fontSize(int size) { return FontSizeManip{ size }; }

	class FontWeightManip : public ManipBase<int> {
	public:
		FontWeightManip(int weight) : ManipBase<int>(weight) {}
		friend Style& operator<<(Style& o, const FontWeightManip& manip)
		{
			return o.setFontWeight(manip.m_data);
		}
	};

	inline FontWeightManip fontWeight(int size) { return FontWeightManip{ size }; }
	inline FontWeightManip bold() { return FontWeightManip{ FW_BOLD }; }

	class FontItalicManip : public ManipBase<bool> {
	public:
		FontItalicManip(bool italic) : ManipBase<bool>(italic) {}
		friend Style& operator<<(Style& o, const FontItalicManip& manip)
		{
			return o.setFontItalic(manip.m_data);
		}
	};

	inline FontItalicManip fontItalic(bool italic) { return FontItalicManip{ italic }; }
	inline FontItalicManip italic() { return FontItalicManip{ true }; }

	class FontUnderlineManip : public ManipBase<bool> {
	public:
		FontUnderlineManip(bool underline) : ManipBase<bool>(underline) {}
		friend Style& operator<<(Style& o, const FontUnderlineManip& manip)
		{
			return o.setFontUnderline(manip.m_data);
		}
	};

	inline FontUnderlineManip fontUnderline(bool underline) { return FontUnderlineManip{ underline }; }
	inline FontUnderlineManip underline() { return FontUnderlineManip{ true }; }

	class FontFamilyManip : public ManipBase<family> {
	public:
		FontFamilyManip(family name) : ManipBase<family>(name) {}
		friend Style& operator<<(Style& o, const FontFamilyManip& manip)
		{
			return o.setFontFamily(manip.m_data);
		}
	};

	inline FontFamilyManip fontFamily(family name) { return FontFamilyManip{ name }; }
	inline FontFamilyManip symbols() { return FontFamilyManip{ family::symbols }; }

	class frameSelect {
	public:
		friend Style& operator<<(Style& o, const frameSelect& /*manip*/)
		{
			return o.setFrameSelect();
		}
	};

	class BackgroundManip : public ManipBase<std::pair<bk, COLORREF>> {
	public:
		BackgroundManip(bk mode, COLORREF color) : ManipBase<std::pair<bk, COLORREF>>(std::make_pair(mode, color)) {}
		friend Style& operator<<(Style& o, const BackgroundManip& manip)
		{
			return o.setBackground(manip.m_data.first, manip.m_data.second);
		}
	};

	inline BackgroundManip background(COLORREF color) { return BackgroundManip{ bk::solid, color }; }

	void Style::apply(Style& style, rules rule, IJiraNode* node)
	{
		switch (rule) {
		case rules::body:
			// from UI
			break;
		case rules::header:
			style
				<< fontSize((style.m_styler.getFontSize() * 18) / 10)
				<< color(0x00883333);
			break;
		case rules::error:
			style << color(0x00171BC1);
			break;
		case rules::tableHead:
			style << bold();
			break;
		case rules::tableRow:
			// same as parent
			if (node && node->getHovered())
				style << background(0x00f8f8f8);
			break;
		case rules::classEmpty:
			style << italic() << color(0x00555555);
			break;
		case rules::classSummary:
			style
				<< fontSize((style.m_styler.getFontSize() * 8) / 10)
				<< color(0x00555555);
			break;
		case rules::symbol:
			style << symbols();
			break;
		};
	}

#if 0
	void serverHeader(Styler& styler, const CTasksView::ServerInfo& item)
	{
		Style style{ styler, rules::header, nullptr };

		auto& server = *item.m_server;

		std::ostringstream o;
		o << server.login() << "@" << server.displayName();
		if (item.m_loading) {
			if (!item.m_gotProgress) {
				o << " ...";
			} else if (item.m_progress.calculable) {
				o << " " << (100 * item.m_progress.loaded / item.m_progress.content) << "%";
			} else {
				o << " " << item.m_progress.loaded << "B";
			}
		}

		styler.out()
			.skipY(0.25) // margin-top: 0.25em
			.println(utf::widen(o.str()))
			.skipY(0.1); // margin-bottom: 0.1em
	}
#endif

	void paintNode(Styler& styler, const std::shared_ptr<jira::node>& node)
	{
		auto& painter = static_cast<IJiraPainter&>(styler.out());
		auto orig = painter.getOrigin();

		styler.out().paint(node, &styler);
		auto size = cast(node)->getSize();
		orig.y += size.height;

		painter.setOrigin(orig);
	}
};

struct StyleSave
{
	Style saved;
	explicit StyleSave(Styler& styler)
		: saved{ styler }
	{
	}

	void apply(jira::styles style, IJiraNode* node)
	{
		using namespace jira;

		switch (style) {
		case styles::unset:
			// from UI
			break;
		case styles::none:
			saved << italic() << color(0x00555555);
			break;
		case styles::error:
			saved << color(0x002600E6);
			break;
		case styles::link:
			saved << color(0x00AF733B);
			if (node && node->getHovered())
				saved << underline();
			if (node && node->getActive())
				saved << frameSelect();
			break;
		};
	}

	void apply(rules rule, IJiraNode* node)
	{
		Style::apply(saved, rule, node);
	}
};

StyleSave* LinePrinter::setStyle(jira::styles style, IJiraNode* node)
{
	if (style == jira::styles::unset)
		return nullptr;

	auto mem = std::make_unique<StyleSave>(*uplink);
	mem->apply(style, node);
	drawBackground(node);
	drawFocus(node);
	return mem.release();
}

StyleSave* LinePrinter::setStyle(rules rule, IJiraNode* node)
{
	auto mem = std::make_unique<StyleSave>(*uplink);
	mem->apply(rule, node);
	drawBackground(node);
	drawFocus(node);
	return mem.release();
}

void LinePrinter::restoreStyle(StyleSave* save)
{
	std::unique_ptr<StyleSave> mem{ save };
}

int LinePrinter::dpiRescale(int size)
{
	return dc.GetDeviceCaps(LOGPIXELSX) * size / 96;
}

LRESULT CTasksView::OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CPaintDC dc(m_hWnd);
	dc.FillRect(&dc.m_ps.rcPaint, m_background);

	dc.SetBkMode(TRANSPARENT);
	Styler control{ (HDC)dc, (HFONT)m_font };
	control.out().updateRect(dc.m_ps.rcPaint);

	for (auto& item : m_servers) {
		if (item.m_plaque)
			paintNode(control, item.m_plaque);
	}

	if (m_cheatsheet)
		paintNode(control, m_cheatsheet);

	//dc.Draw3dRect(m_mouseX - 3, m_mouseY - 3, 6, 6, 0x00000080, 0x00000080);

	//if (m_active)
	//	paintGrayFrame((HDC)dc, 0x00, m_active);

	return 0;
}

void CTasksView::updateLayout()
{
	CWindowDC dc{m_hWnd};
	Styler styler{ (HDC) dc, (HFONT) m_font };

	if (m_hovered)
		cast(m_hovered)->setHovered(false);
	m_hovered = nullptr;

	if (m_active)
		cast(m_active)->setActive(false);
	m_active = nullptr;

	size_t height = 0;
	size_t width = 0;
	for (auto& server : m_servers) {
		if (server.m_plaque) {
			styler.out().measure(server.m_plaque, &styler);
			auto size = cast(server.m_plaque)->getSize();
			cast(server.m_plaque)->setPosition(BODY_MARGIN, BODY_MARGIN + height);
			height += size.height;
			if (width < size.width)
				width = size.width;
		}
	}

	if (m_cheatsheet) {
		styler.out().measure(m_cheatsheet, &styler);
		auto size = m_cheatsheet->getSize();
		m_cheatsheet->setPosition(BODY_MARGIN, BODY_MARGIN + height);
		height += size.height;
		if (width < size.width)
			width = size.width;
	}

	setDocumentSize(width + 2 * BODY_MARGIN, height + 2 * BODY_MARGIN);

	m_hovered = nodeFromPoint();
	if (m_hovered)
		cast(m_hovered)->setHovered(true);
	updateCursorAndTooltip();
}

void CTasksView::updateCursor(bool force)
{
	auto tmp = cursor::arrow;
	if (m_hovered)
		tmp = cast(m_hovered)->getCursor();

	if (tmp == cursor::inherited)
		tmp = cursor::arrow;

	if (tmp == m_cursor && !force)
		return;

	m_cursor = tmp;
	LPCWSTR idc = IDC_ARROW;
	switch (m_cursor) {
	case cursor::pointer:
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
		if (cast(node)->hasTooltip()) {
			auto pt = cast(m_hovered)->getAbsolutePos();
			auto sz = cast(m_hovered)->getSize();
			RECT r{ pt.x - 2, pt.y - 2, pt.x + (int)sz.width + 4 , pt.y + (int)sz.height + 4 };
			tool = r;
			tooltip = cast(node)->getTooltip();
			break;
		}

		node = cast(node)->getParent();
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

void CTasksView::updateCursorAndTooltip(bool force)
{
	updateCursor(force);
	updateTooltip(force);
}

std::shared_ptr<jira::node> CTasksView::nodeFromPoint()
{
	for (auto& server : m_servers) {
		if (!server.m_plaque)
			continue;

		auto tmp = cast(server.m_plaque)->nodeFromPoint(m_mouseX, m_mouseY);
		if (tmp)
			return tmp;
	}

	return{};
}

void CTasksView::setDocumentSize(size_t width, size_t height)
{
	if (m_scroller)
		m_scroller(width, height);
}

LRESULT CTasksView::OnSetFont(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
	bHandled = FALSE;
	m_font = (HFONT) wParam;

	updateLayout();
	// TODO: redraw the report table

	return 0;
}

LRESULT CTasksView::OnMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
	//RECT r{ m_mouseX - 4, m_mouseY - 4, m_mouseX + 4, m_mouseY + 4 };
	//InvalidateRect(&r);
	m_mouseX = GET_X_LPARAM(lParam);
	m_mouseY = GET_Y_LPARAM(lParam);
	//RECT r2{ m_mouseX - 4, m_mouseY - 4, m_mouseX + 4, m_mouseY + 4 };
	//InvalidateRect(&r2);

	auto tmp = nodeFromPoint();
	if (tmp != m_hovered) {
		if (tmp)
			cast(tmp)->setHovered(true);
		if (m_hovered)
			cast(m_hovered)->setHovered(false);

		m_hovered = tmp;
		updateCursorAndTooltip();
	}
	return 0;
}

LRESULT CTasksView::OnMouseDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
	m_mouseX = GET_X_LPARAM(lParam);
	m_mouseY = GET_Y_LPARAM(lParam);

	auto tmp = m_active;
	m_active = nodeFromPoint();
	if (m_active)
		cast(m_active)->setActive(true);
	if (tmp)
		cast(tmp)->setActive(false);

	m_tracking = true;
	SetCapture();
	Invalidate();

	return 0;
}

LRESULT CTasksView::OnMouseUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
	m_mouseX = GET_X_LPARAM(lParam);
	m_mouseY = GET_Y_LPARAM(lParam);
	auto tmp = nodeFromPoint();

	m_tracking = false;
	ReleaseCapture();
	Invalidate();

	if (tmp && tmp == m_active)
		cast(m_active)->activate();

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
				auto sit = std::find_if(std::begin(servers), std::end(servers), [&](const std::shared_ptr<jira::server>& server) { return server->sessionId() == wParam; });
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
				auto query = std::find_if(std::begin(servers), std::end(servers), [&](const std::shared_ptr<jira::server>& srv) { return srv->sessionId() == it->m_sessionId; });
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
				if (it != end && (*srv_it)->sessionId() == it->m_sessionId)
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
	it->m_plaque = std::make_unique<CJiraReportElement>(it->m_dataset, make_invalidator(m_hWnd));
	std::static_pointer_cast<CJiraReportElement>(it->m_plaque)->addChildren(server);
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
