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
	m_plaque = std::make_unique<CJiraReportElement>(m_dataset, *server, make_invalidator(hWnd));
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

class LinePrinter : public IJiraPainter
{
	HFONT older;
	CFontHandle font;
	CDCHandle dc;
	long lineHeight;
	int x = BODY_MARGIN;
	int y = BODY_MARGIN;
	Styler* uplink = nullptr;
	bool selectedFrame = false;

	void updateLineHeight()
	{
		TEXTMETRIC metric = {};
		dc.GetTextMetrics(&metric);
		lineHeight = metric.tmHeight * 12 / 10; // 120%
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
		dc.TextOut(x, y, utf::widen(text).c_str());
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

	LinePrinter& println(const std::wstring& line)
	{
		if (!line.empty())
			dc.TextOut(x, y, line.c_str());
		y += lineHeight;
		x = BODY_MARGIN;
		return *this;
	}

	LinePrinter& print(const std::wstring& line)
	{
		if (line.empty())
			return *this;
		SIZE s = {};
		dc.GetTextExtent(line.c_str(), line.length(), &s);
		dc.TextOut(x, y, line.c_str());
		x += s.cx;
		return *this;
	}

	LinePrinter& skipY(double scale)
	{
		y += int(lineHeight * scale);
		x = BODY_MARGIN;
		return *this;
	}

	LinePrinter& moveToX(int x_)
	{
		x = x_;
		return *this;
	}

	LinePrinter& drawFocus(jira::node* node)
	{
		if (selectedFrame) {
			selectedFrame = false;
			auto pt = static_cast<IJiraNode*>(node)->getAbsolutePos();
			auto sz = static_cast<IJiraNode*>(node)->getSize();
			RECT r{pt.x - 2, pt.y - 2, pt.x + (int)sz.width + 4 , pt.y + (int)sz.height + 4 };
			dc.DrawFocusRect(&r);
		}
		return *this;
	}

	LinePrinter& paint(const std::unique_ptr<jira::node>& node, Styler* link)
	{
		uplink = link;
		cast(node)->paint(this);
		return *this;
	}

	LinePrinter& measure(const std::unique_ptr<jira::node>& node, Styler* link)
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

	int getX() const { return x; }
	CDCHandle native() const { return dc; }
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

	void Style::apply(Style& style, rules rule, IJiraNode* /*node*/)
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
			break;
		case rules::classEmpty:
			style << italic() << color(0x00555555);
			break;
		case rules::classSummary:
			style
				<< fontSize((style.m_styler.getFontSize() * 8) / 10)
				<< color(0x00555555);
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

	void plaque(Styler& styler, const std::unique_ptr<jira::node>& node)
	{
		auto& painter = static_cast<IJiraPainter&>(styler.out());
		auto orig = painter.getOrigin();

		styler.out().paint(node, &styler);
		auto size = cast(node)->getSize();
		orig.y += size.height;

		painter.setOrigin(orig);
	}

	void server(Styler& styler, const CTasksView::ServerInfo& item)
	{
		if (item.m_plaque)
			plaque(styler, item.m_plaque);
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
	drawFocus(node);
	return mem.release();
}

StyleSave* LinePrinter::setStyle(rules rule, IJiraNode* node)
{
	auto mem = std::make_unique<StyleSave>(*uplink);
	mem->apply(rule, node);
	drawFocus(node);
	return mem.release();
}

void LinePrinter::restoreStyle(StyleSave* save)
{
	std::unique_ptr<StyleSave> mem{ save };
}

IJiraPainter::point getOffset(jira::node* node) {
	if (!node)
		return{ 0, 0 };

	auto pt = static_cast<IJiraNode*>(node)->getPosition();
	auto parent = static_cast<IJiraNode*>(node)->getParent();

	auto tmp = getOffset(parent);

	pt.x += tmp.x;
	pt.y += tmp.y;
	return pt;
}

IJiraPainter::point paintGrayFrame(CDCHandle dc, BYTE shade, jira::node* node) {
	if (!node)
		return{ 0, 0 };

	auto pt = static_cast<IJiraNode*>(node)->getPosition();
	auto sz = static_cast<IJiraNode*>(node)->getSize();
	auto parent = static_cast<IJiraNode*>(node)->getParent();

	uint16_t channel = shade + 0x44;
	if (channel >= 0xFF) {
		auto tmp = getOffset(parent);
		pt.x += tmp.x;
		pt.y += tmp.y;
	} else {
		auto tmp = paintGrayFrame(dc, (BYTE)channel, parent);
		pt.x += tmp.x;
		pt.y += tmp.y;
	}

	RECT r{ pt.x, pt.y, pt.x + (int)sz.width, pt.y + (int)sz.height };
	dc.Draw3dRect(pt.x, pt.y, sz.width, sz.height, RGB(shade, shade, shade), RGB(shade, shade, shade));

	return pt;
}

LRESULT CTasksView::OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CPaintDC dc(m_hWnd);
	dc.FillRect(&dc.m_ps.rcPaint, m_background);

	dc.SetBkMode(TRANSPARENT);
	Styler control{ (HDC)dc, (HFONT)m_font };

#if FA_CHEATSHEET
	std::wstring textFace;
	std::vector<std::pair<size_t, wchar_t>> ranges;
	{
		Style style{ control };
		style << symbols();

		{
			auto textLen = dc.GetTextFaceLen();
			std::unique_ptr<wchar_t[]> text{ new wchar_t[textLen] };
			dc.GetTextFace(text.get(), textLen);
			textFace = text.get();
		}

		{
			auto size = dc.GetFontUnicodeRanges(nullptr);
			std::unique_ptr<char[]> glyphsPtr{ new char[size] };
			auto glyphs = reinterpret_cast<GLYPHSET*>(glyphsPtr.get());
			dc.GetFontUnicodeRanges(glyphs);

			for (DWORD i = 0; i < glyphs->cRanges; ++i) {
				if (!ranges.empty()) {
					auto& range = ranges.back();
					auto last = std::get<0>(range) + std::get<1>(range);
					if (last == glyphs->ranges[i].wcLow) {
						std::get<0>(range) += glyphs->ranges[i].cGlyphs;
						continue;
					}
				}
				ranges.emplace_back(glyphs->ranges[i].cGlyphs, glyphs->ranges[i].wcLow);
			}
		}
	}
#endif

	for (auto& item : m_servers)
		server(control, item);

#if FA_CHEATSHEET
	control.out().println({}).println(L"Font face: " + textFace);

	size_t pos = 0;
	for (size_t id = 0; id < (size_t)fa::glyph::__last_glyph; ++id, ++pos) {
		wchar_t s[2] = {};
		s[0] = fa::glyph_char((fa::glyph)id);
		s[1] = 0;
		{
			Style style{ control };
			style << symbols();
			control.out().print(s);
		}

		{
			Style style{ control };
			style << color(0x0060c060) << fontSize((control.getFontSize() * 8) / 10);
			control.out()
				.print(L" ")
				.print(utf::widen(fa::glyph_name((fa::glyph)id)))
				.print(L", ");
		}

		if (pos == 16) {
			control.out().println({});
			pos = 0;
		}
	}

	Style style{ control };
	style << symbols() << fontSize((control.getFontSize() * 18) / 10);

	pos = 0;
	std::wstring line;

	for (auto& range : ranges) {
		for (USHORT cnt = 0; cnt < std::get<0>(range); ++cnt) {
			WCHAR glyph = cnt + std::get<1>(range);
			line.push_back(glyph);
			line.push_back(' ');
			pos++;
			if (pos == 32) {
				control.out().println(line);
				line.clear();
				pos = 0;
			}
		}
	}

	if (pos) {
		control.out().println(line);
		line.clear();
	}
#endif

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
		static_cast<IJiraNode*>(m_hovered)->setHovered(false);
	m_hovered = nullptr;

	if (m_active)
		static_cast<IJiraNode*>(m_active)->setActive(false);
	m_active = nullptr;

	int height = 0;
	int width = 0;
	for (auto& server : m_servers) {
		if (server.m_plaque) {
			styler.out().measure(server.m_plaque, &styler);
			auto size = cast(server.m_plaque)->getSize();
			cast(server.m_plaque)->setPosition(BODY_MARGIN, BODY_MARGIN + height);
			height += size.height;
		}
	}

	// document size: height + 2xBODY_MARGIN, width + 2xBODY_MARGIN

	m_hovered = nodeFromPoint();
	if (m_hovered)
		static_cast<IJiraNode*>(m_hovered)->setHovered(true);
	updateCursorAndTooltip();
}

void CTasksView::updateCursor(bool force)
{
	auto tmp = cursor::arrow;
	if (m_hovered)
		tmp = static_cast<IJiraNode*>(m_hovered)->getCursor();

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
		if (static_cast<IJiraNode*>(node)->hasTooltip()) {
			auto pt = static_cast<IJiraNode*>(m_hovered)->getAbsolutePos();
			auto sz = static_cast<IJiraNode*>(m_hovered)->getSize();
			RECT r{ pt.x - 2, pt.y - 2, pt.x + (int)sz.width + 4 , pt.y + (int)sz.height + 4 };
			tool = r;
			tooltip = static_cast<IJiraNode*>(node)->getTooltip();
			break;
		}

		node = static_cast<IJiraNode*>(node)->getParent();
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

		OutputDebugString(utf::widen("Local tooltip: " + tooltip + "\n").c_str());
	}
}

void CTasksView::updateCursorAndTooltip(bool force)
{
	updateCursor(force);
	updateTooltip(force);
}

jira::node* CTasksView::nodeFromPoint()
{
	for (auto& server : m_servers) {
		if (!server.m_plaque)
			continue;

		auto tmp = cast(server.m_plaque)->nodeFromPoint(m_mouseX, m_mouseY);
		if (tmp)
			return tmp;
	}

	return nullptr;
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
			static_cast<IJiraNode*>(tmp)->setHovered(true);
		if (m_hovered)
			static_cast<IJiraNode*>(m_hovered)->setHovered(false);

		m_hovered = tmp;
		updateCursorAndTooltip();
		Invalidate(); // TODO: invalidate old and new hovered/their parents...
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
		static_cast<IJiraNode*>(m_active)->setActive(true);
	if (tmp)
		static_cast<IJiraNode*>(tmp)->setActive(false);

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
		static_cast<IJiraNode*>(m_active)->activate();

	return 0;
}

LRESULT CTasksView::OnSetCursor(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
{
	bHandled = LOWORD(lParam) == HTCLIENT;
	if (bHandled) {
		SetCursor(m_cursorObj);
	}

	return FALSE;
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
	it->m_plaque = std::make_unique<CJiraReportElement>(it->m_dataset, server, make_invalidator(m_hWnd));
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
