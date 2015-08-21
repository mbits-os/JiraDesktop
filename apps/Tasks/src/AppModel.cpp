#include "stdafx.h"
#include "AppModel.h"
#include "AppSettings.h"
#include "XHRConstructor.h"
#include <net/post_mortem.hpp>
#include <algorithm>

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

	void start(const std::shared_ptr<gui::document>& doc, const std::shared_ptr<jira::server>& srv, const std::string& uri);
};

CAppModel::CAppModel()
{
}

void CAppModel::onListChanged(uint32_t addedOrRemoved)
{
	emit([addedOrRemoved](CAppModelListener* listener) { listener->onListChanged(addedOrRemoved); });
}

class ImageCreator: public gui::image_creator {
	std::shared_ptr<jira::server> m_server;
	std::weak_ptr<gui::document> m_doc;
public:
	explicit ImageCreator(const std::shared_ptr<jira::server>& srvr)
		: m_server(srvr)
	{
	}

	void setDoc(const std::shared_ptr<gui::document>& doc) { m_doc = doc; }

	std::shared_ptr<gui::image_ref> create(const std::string& uri) override
	{
		auto doc = m_doc.lock();
		if (!doc)
			return { };
		auto ref = std::make_shared<CJiraImageRef>();
		ref->start(doc, m_server, uri);
		return ref;
	}
};

std::shared_ptr<gui::document> make_document(const std::shared_ptr<jira::server>& srvr, const gui::credential_ui_ptr& ui)
{
	auto imgCreator = std::make_shared<ImageCreator>(srvr);
	auto xhrCtor = std::make_shared<XHRConstructor>();
	auto doc = gui::document::make_doc(imgCreator, xhrCtor, ui);
	imgCreator->setDoc(doc);
	xhrCtor->setDoc(doc);
	return doc;
}

void CAppModel::startup(const gui::credential_ui_ptr& cred_ui)
{
	m_cred_ui = cred_ui;
	synchronize(m_guard, [&] {
		CAppSettings settings;
		auto servers = settings.jiraServers();
		m_servers.clear();
		m_servers.reserve(servers.size());
		for (auto server : servers) {
			ServerInfo info{ server, make_document(server, m_cred_ui) };
			m_servers.push_back(std::move(info));
		}
	});

	onListChanged(0);

	auto local = m_servers;
	for (auto server : local) {
		pm::thread( [server] {
			server.m_server->loadFields(server.m_document);
			server.m_server->refresh(server.m_document);
		} ).detach();
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
		ServerInfo info{ server, make_document(server, m_cred_ui) };
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
		pm::thread( [server, document] {
			server->loadFields(document);
			server->refresh(document);
		} ).detach();
	}
}

void CAppModel::startTimer(uint32_t sessionId)
{
	for (auto& server : m_servers) {
		auto& views = server.m_server->views();
		for (auto& view : views) {
			if (view.sessionId() != sessionId)
				continue;

			auto timeout = view.timeout();
			if (timeout == std::chrono::milliseconds::max())
				timeout = jira::search_def::standard.timeout();

			if (timeout.count() > 0 && timeout.count() < std::numeric_limits<UINT>::max())
				SetTimer(m_hwndTimer, sessionId, (UINT)timeout.count(), nullptr);

			return;
		}
	}
}

CJiraImageRef::CJiraImageRef()
{
}

CJiraImageRef::~CJiraImageRef()
{
	delete m_bitmap;
}

void CJiraImageRef::start(const std::shared_ptr<gui::document>& doc, const std::shared_ptr<jira::server>& srv, const std::string& uri)
{
	using namespace net::http::client;
	auto local = shared_from_this();
	m_state = gui::load_state::image_missing;
	srv->get(doc, uri, [srv, uri, local](XmlHttpRequest* req) {
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
