#include "stdafx.h"
#include "AppModel.h"
#include "AppSettings.h"
#include <thread>
#include <net/xhr.hpp>
#include <net/utf8.hpp>
#include <gui/nodes.hpp>
#include <sstream>
#include <algorithm>

CJiraReportElement::CJiraReportElement(const std::shared_ptr<jira::report>& dataset)
	: gui::block_node(gui::elem::block)
	, m_dataset(dataset)
{
}

void CJiraReportElement::addChildren(const jira::server& server)
{
	{
		auto block = std::make_shared<gui::span_node>(gui::elem::header);
		block->innerText(server.login() + "@" + server.displayName());
		node_base::addChild(block);
	}

	for (auto& error : server.errors()) {
		auto note = std::make_shared<gui::span_node>(gui::elem::span);
		note->innerText(error);
		note->addClass("error");
		node_base::addChild(std::move(note));
	}

	auto dataset = m_dataset.lock();
	if (dataset) {
		auto table = std::make_shared<gui::table_node>();

		{
			auto caption = std::make_shared<gui::caption_row_node>();
			auto jql = server.view().jql();
			if (jql.empty())
				jql = jira::search_def::standard.jql();
			caption->innerText(jql);
			table->addChild(caption);
		}
		{
			auto header = std::make_shared<gui::row_node>(gui::elem::table_head);
			for (auto& col : dataset->schema.cols()) {
				auto name = col->title();
				auto th = std::make_shared<gui::span_node>(gui::elem::th);
				th->innerText(name);

				auto tooltip = col->titleFull();
				if (name != tooltip)
					th->setTooltip(tooltip);

				header->addChild(th);
			}
			table->addChild(header);
		}

		for (auto& record : dataset->data)
			table->addChild(record.getRow());

		node_base::addChild(table);

		std::ostringstream o;
		auto low = dataset->data.empty() ? 0 : 1;
		o << "(Issues " << (dataset->startAt + low)
			<< '-' << (dataset->startAt + dataset->data.size())
			<< " of " << dataset->total << ")";

		auto note = std::make_shared<gui::span_node>(gui::elem::span);
		note->innerText(o.str());
		note->addClass("summary");
		node_base::addChild(std::move(note));
	}
	else {
		auto note = std::make_shared<gui::span_node>(gui::elem::span);
		note->innerText("Empty");
		note->addClass("empty");
		node_base::addChild(std::move(note));
	}
}

void CJiraReportElement::addChild(const std::shared_ptr<node>& /*child*/)
{
	// noop
}

class CJiraImageRef : public gui::image_ref, public std::enable_shared_from_this<CJiraImageRef> {
	gui::load_state getState() const override { return m_state; }
	size_t getWidth() const override { return m_width; }
	size_t getHeight() const override { return m_height; }
	void* getNativeHandle() const override { return m_bitmap; }

	gui::load_state m_state = gui::load_state::image_missing;
	size_t m_width = 0;
	size_t m_height = 0;

	Gdiplus::Bitmap* m_bitmap = nullptr;
public:
	explicit CJiraImageRef();
	~CJiraImageRef();

	void start(const std::shared_ptr<jira::server>& srv, const std::string& uri);
};

CAppModel::CAppModel()
	//: m_document(std::make_shared<gui::document_base>(image_creator))
{
}

void CAppModel::onListChanged(uint32_t addedOrRemoved)
{
	emit([addedOrRemoved](CAppModelListener* listener) { listener->onListChanged(addedOrRemoved); });
}

class ImageCreator: public gui::image_creator {
	std::shared_ptr<jira::server> m_server;
public:
	explicit ImageCreator(const std::shared_ptr<jira::server>& srvr)
		: m_server(srvr)
	{
	}

	std::shared_ptr<gui::image_ref> create(const std::string& uri) override
	{
		auto ref = std::make_shared<CJiraImageRef>();
		ref->start(m_server, uri);
		return ref;
	}
};

std::shared_ptr<gui::document> make_document(const std::shared_ptr<jira::server>& srvr)
{
	return std::make_shared<gui::document_base>(std::make_shared<ImageCreator>(srvr));
}

void CAppModel::startup()
{
	synchronize(m_guard, [&] {
		CAppSettings settings;
		auto servers = settings.jiraServers();
		m_servers.clear();
		m_servers.reserve(servers.size());
		for (auto server : servers) {
			ServerInfo info{ server, make_document(server) };
			m_servers.push_back(std::move(info));
		}
	});

	onListChanged(0);

	auto local = m_servers;
	for (auto server : local) {
		std::thread{ [server] {
			server.m_server->loadFields();
			server.m_server->refresh(server.m_document);
		} }.detach();
	}
}

void CAppModel::lock()
{
	m_guard.lock();
}

void CAppModel::unlock()
{
	m_guard.unlock();
}

const std::vector<ServerInfo>& CAppModel::servers() const
{
	return m_servers;
}

void CAppModel::add(const std::shared_ptr<jira::server>& server)
{
	if (!server)
		return;

	synchronize(m_guard, [&]{
		ServerInfo info{ server, make_document(server) };
		m_servers.push_back(std::move(info));
	});
	onListChanged(server->sessionId());

	update(server);
}

void CAppModel::remove(const std::shared_ptr<jira::server>& server)
{
	auto id = server ? server->sessionId() : 0;
	if (server) {
		synchronize(m_guard, [&] {
			auto it = m_servers.begin();
			auto end = m_servers.end();
			for (; it != end; ++it) {
				auto& server = *it;
				if (server.m_server && server.m_server->sessionId() == id) {
					m_servers.erase(it);
					break;
				}
			}

			std::vector<std::shared_ptr<jira::server>> list;
			list.reserve(m_servers.size());
			for (auto& info : m_servers)
				list.push_back(info.m_server);

			CAppSettings settings;
			settings.jiraServers(list);
		});
	}

	onListChanged(id);
}

void CAppModel::update(const std::shared_ptr<jira::server>& server)
{
	if (!server)
		return;

	synchronize(m_guard, [&] {
		std::vector<std::shared_ptr<jira::server>> list;
		list.reserve(m_servers.size());
		for (auto& info : m_servers)
			list.push_back(info.m_server);

		CAppSettings settings;
		settings.jiraServers(list);
	});

	auto it = std::find_if(std::begin(m_servers), std::end(m_servers), [&](const ServerInfo& info) {
		return server->sessionId() == info.m_server->sessionId();
	});

	if (it != m_servers.end()) {
		auto document = it->m_document;
		std::thread{ [server, document] {
			server->loadFields();
			server->refresh(document);
		} }.detach();
	}
}

CJiraImageRef::CJiraImageRef()
{
}

CJiraImageRef::~CJiraImageRef()
{
	delete m_bitmap;
}

void CJiraImageRef::start(const std::shared_ptr<jira::server>& srv, const std::string& uri)
{
	using namespace net::http::client;
	auto local = shared_from_this();
	m_state = gui::load_state::image_missing;
	srv->get(uri, [srv, uri, local](XmlHttpRequest* req) {
		auto thiz = local.get();
		if ((req->getStatus() / 100) != 2) {
			thiz->emit([thiz](gui::image_ref_callback* cb) { cb->onImageChange(thiz); });
			return;
		}

		CComPtr<IStream> stream;
		auto ptr = SHCreateMemStream(
			(const BYTE*)req->getResponseText(),
			req->getResponseTextLength());
		stream.Attach(ptr); // this will not increase the counter

		thiz->m_bitmap = Gdiplus::Bitmap::FromStream(stream);

		if (thiz->m_bitmap)
			thiz->m_state = gui::load_state::pixmap_available;

		thiz->emit([thiz](gui::image_ref_callback* cb) { cb->onImageChange(thiz); });
	});
}
