// TasksView.cpp : implementation of the CTasksView class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "TasksView.h"
#include <net/utf8.hpp>
#include <algorithm>
#include <sstream>

class TaskViewModelListener : public CAppModelListener {
	HWND m_hWnd;
public:
	TaskViewModelListener(HWND hWnd) : m_hWnd(hWnd) {}

	void onListChanged(uint32_t addedOrRemoved) override
	{
		// traversing the borders of threads, if needed
		// also, the actual reaction to the vent will
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

void CTasksView::ServerInfo::calcColumns(CDCHandle dc, CFontHandle text, CFontHandle header)
{
	if (!m_dataset) {
		m_columns.clear();
		return;
	}

	auto older = dc.SelectFont(header);
	SIZE s = {};

	m_columns.resize(m_dataset->schema.cols().size());
	auto dst = m_columns.begin();
	for (auto& col : m_dataset->schema.cols()) {
		auto title = utf::widen(col->titleFull());
		dc.GetTextExtent(title.c_str(), title.length(), &s);
		*dst++ = s.cx;
	}

	dc.SelectFont(text);
	for (auto& item : m_dataset->data) {
		dst = m_columns.begin();
		for (auto& value : item.values()) {
			auto txt = utf::widen(value->text());
			dc.GetTextExtent(txt.c_str(), txt.length(), &s);
			if (s.cx > *dst)
				*dst++ = s.cx;
			else ++dst;
		}
	}
	dc.SelectFont(older);
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
	return m_servers.emplace(it, server, listener);
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
	BODY_MARGIN = 7,
	CELL_MARGIN = 7
};

class LinePrinter
{
	HFONT older;
	CFontHandle font;
	CDCHandle dc;
	long lineHeight;
	int x = BODY_MARGIN;
	int y = BODY_MARGIN;

	void updateLineHeight()
	{
		TEXTMETRIC metric = {};
		dc.GetTextMetrics(&metric);
		lineHeight = metric.tmHeight * 12 / 10; // 120%
	}
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

	int getX() const { return x; }
};

namespace {

	enum class rules {
		body,
		header,
		tableHead,
		tableRow,
		classEmpty,
		classSummary
	};

	class Styler {
		LinePrinter printer;
		CDCHandle dc;
		CFont font;
		LOGFONT logFont;
		COLORREF lastColor;

		void update()
		{
			font.CreateFontIndirect(&logFont);
			printer.select(font);
		}
	public:
		Styler(HDC dc_, HFONT font_)
			: printer(dc_, font_)
			, dc(dc_)
			, lastColor(0)
		{
			CFontHandle{ font_ }.GetLogFont(&logFont);
			lastColor = dc.GetTextColor();
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

		COLORREF getColor() const { return lastColor; }
		bool getFontItalic() const { return !!logFont.lfItalic; }
		int getFontWeight() const { return logFont.lfWeight; }
		int getFontSize() const { return logFont.lfHeight; }
		LinePrinter& out() { return printer; }

		const LOGFONT& fontDef() const { return this->logFont; }
	};


	class Style {
		Styler& m_styler;
		COLORREF m_color;
		int m_weight;
		int m_size;
		bool m_italic;

		static void apply(Style& style, rules rule);

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
		{
		}

		explicit Style(Styler& styler, rules rule)
			: m_styler(styler)
			, m_color(styler.getColor())
			, m_weight(styler.getFontWeight())
			, m_size(styler.getFontSize())
			, m_italic(styler.getFontItalic())
		{
			apply(*this, rule);
		}

		~Style()
		{
			setColor(m_color);
			setFontWeight(m_weight);
			setFontSize(m_size);
			setFontItalic(m_italic);
		}

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


	void Style::apply(Style& style, rules rule)
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


	void serverHeader(Styler& styler, const CTasksView::ServerInfo& item)
	{
		Style style{ styler, rules::header };

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

	void tableHead(Styler& styler, const jira::model& schema, const std::vector<int>& widths)
	{
		Style style{ styler, rules::tableHead };

		auto x = styler.out().getX();
		auto src = widths.begin();
		for (auto& col : schema.cols()) {
			styler.out().moveToX(x).print(utf::widen(col->titleFull()));
			x += *src++ + CELL_MARGIN + CELL_MARGIN;
		}

		styler.out().println({});
	}

	void tableRow(Styler& styler, const jira::record& row, const std::vector<int>& widths)
	{
		Style style{ styler, rules::tableRow };

		auto x = styler.out().getX();
		auto src = widths.begin();
		for (auto& value : row.values()) {
			styler.out().moveToX(x).print(utf::widen(value->text()));
			x += *src++ + CELL_MARGIN + CELL_MARGIN;
		}

		styler.out().println({});
	}

	void tableFoot(Styler& styler, const jira::report& dataset)
	{
		Style style{ styler, rules::classSummary };

		std::ostringstream o;
		o << "(Issues " << (dataset.startAt + 1)
			<< '-' << (dataset.startAt + dataset.data.size())
			<< " of " << dataset.total << ")";
		styler.out()/*.println({})*/.println(utf::widen(o.str()).c_str()); o.str("");
	}

	void table(Styler& styler, const jira::report& dataset, const std::vector<int>& widths)
	{
		tableHead(styler, dataset.schema, widths);
		for (auto&& row : dataset.data)
			tableRow(styler, row, widths);
		tableFoot(styler, dataset);
	}

	void noTable(Styler& styler)
	{
		Style style{ styler, rules::classEmpty };

		styler.out().println(L"Empty");
	}

	void server(Styler& styler, const CTasksView::ServerInfo& item)
	{
		serverHeader(styler, item);
		if (item.m_dataset)
			table(styler, *item.m_dataset, item.m_columns);
		else
			noTable(styler);
	}

	HFONT getFont(Styler& styler, rules rule)
	{
		Style style{ styler, rule };
		CFont font;
		font.CreateFontIndirect(&styler.fontDef());
		return font.Detach();
	}
};

LRESULT CTasksView::OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CPaintDC dc(m_hWnd);
	dc.FillRect(&dc.m_ps.rcPaint, m_background);

	dc.SetBkMode(TRANSPARENT);
	Styler control{ (HDC) dc, (HFONT) m_font };

	for (auto& item : m_servers)
		server(control, item);

	return 0;
}

void CTasksView::updateLayout()
{
	CWindowDC dc{m_hWnd};
	Styler styler{ (HDC) dc, (HFONT) m_font };
	CFont row{ getFont(styler, rules::tableRow) };
	CFont header{ getFont(styler, rules::tableHead) };

	for (auto& server : m_servers)
		server.calcColumns((HDC)dc, (HFONT) row, (HFONT) header);
}

LRESULT CTasksView::OnSetFont(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
	bHandled = FALSE;
	m_font = (HFONT) wParam;

	updateLayout();
	// TODO: redraw the report table

	return 0;
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
				auto rit = std::find_if(std::rbegin(servers), std::rend(servers), [&](const std::shared_ptr<jira::server>& server) { return server->sessionId() == wParam; });
				if (rit != std::rend(servers)) {
					auto it = std::begin(m_servers);

					// it += (rit.base() - servers.begin())
					 std::advance(it, std::distance(std::begin(servers), rit.base()));

					 insert(it, *rit);
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

			for (; srv_it != srv_end; ++srv_it) {
				if (it != end && (*srv_it)->sessionId() == it->m_sessionId) {
					++it;
					continue;
				}

				it = insert(it, *srv_it); // fill in blanks...
				end = std::end(m_servers);
			}
		});
	}
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