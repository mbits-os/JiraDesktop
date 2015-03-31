#include "stdafx.h"
#include "AppModel.h"
#include "AppNodes.h"
#include "AppSettings.h"
#include <thread>
#include <net/xhr.hpp>

class CJiraImageRef : public ImageRef, public std::enable_shared_from_this<CJiraImageRef> {
	load_state getState() const override { return m_state; }
	size_t getWidth() const override { return m_width; }
	size_t getHeight() const override { return m_height; }
	void* getNativeHandle() const override { return m_bitmap; }

	load_state m_state = load_state::image_missing;
	size_t m_width = 0;
	size_t m_height = 0;

	Gdiplus::Bitmap* m_bitmap = nullptr;
public:
	explicit CJiraImageRef();
	~CJiraImageRef();

	void start(const std::shared_ptr<jira::server>& srv, const std::string& uri);
};

CAppModel::CAppModel()
	: m_document(std::make_shared<CJiraDocument>(image_creator))
{
}

void CAppModel::onListChanged(uint32_t addedOrRemoved)
{
	emit([addedOrRemoved](CAppModelListener* listener) { listener->onListChanged(addedOrRemoved); });
}

std::shared_ptr<ImageRef> CAppModel::image_creator(const std::shared_ptr<jira::server>& srv, const std::string& uri)
{
	auto ref = std::make_shared<CJiraImageRef>();
	ref->start(srv, uri);
	return ref;
}

void CAppModel::startup()
{
	synchronize(m_guard, [&] {
		CAppSettings settings;
		m_servers = settings.jiraServers();
	});

	onListChanged(0);

	auto local = m_servers;
	auto document = m_document;
	for (auto server : local) {
		std::thread{ [server, document] {
			server->loadFields();
			server->refresh(document);
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

const std::vector<std::shared_ptr<jira::server>>& CAppModel::servers() const
{
	return m_servers;
}

const std::shared_ptr<jira::document>& CAppModel::document() const
{
	return m_document;
}

void CAppModel::add(const std::shared_ptr<jira::server>& server)
{
	if (!server)
		return;

	synchronize(m_guard, [&]{
		m_servers.push_back(server);
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
				if (server && server->sessionId() == id) {
					m_servers.erase(it);
					break;
				}
			}

			CAppSettings settings;
			settings.jiraServers(m_servers);
		});
	}

	onListChanged(id);
}

void CAppModel::update(const std::shared_ptr<jira::server>& server)
{
	if (!server)
		return;

	synchronize(m_guard, [&] {
		CAppSettings settings;
		settings.jiraServers(m_servers);
	});

	auto document = m_document;
	std::thread{ [server, document] {
		server->loadFields();
		server->refresh(document);
	} }.detach();
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
	m_state = load_state::image_missing;
	srv->get(uri, [srv, uri, local](XmlHttpRequest* req) {
		auto thiz = local.get();
		if ((req->getStatus() / 100) != 2) {
			thiz->emit([thiz](ImageRefCallback* cb) { cb->onImageChange(thiz); });
			return;
		}

		CComPtr<IStream> stream;
		auto ptr = SHCreateMemStream(
			(const BYTE*)req->getResponseText(),
			req->getResponseTextLength());
		stream.Attach(ptr); // this will not increase the counter

		thiz->m_bitmap = Gdiplus::Bitmap::FromStream(stream);

		if (thiz->m_bitmap)
			thiz->m_state = load_state::pixmap_available;

		thiz->emit([thiz](ImageRefCallback* cb) { cb->onImageChange(thiz); });
	});
}
