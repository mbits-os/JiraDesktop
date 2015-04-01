#pragma once

#include <jira\server.hpp>
#include <jira\listeners.hpp>

struct CAppModelListener {
	virtual ~CAppModelListener() {}
	virtual void onListChanged(uint32_t addedOrRemoved) = 0;
};

template <typename T, typename F>
inline void synchronize(T& mtx, F fn)
{
	std::lock_guard<T> guard(mtx);
	fn();
}

struct StyleSave;

enum class load_state {
	image_missing,
	size_known,
	pixmap_available
};

enum class rules {
	body,
	header,
	error,
	tableHead,
	tableRow,
	classEmpty,
	classSummary,
};

struct ImageRef;

struct ImageRefCallback {
	virtual ~ImageRefCallback() {}
	virtual void onImageChange(ImageRef*) = 0;
};

struct ImageRef : public jira::listeners<ImageRefCallback, ImageRef> {
	virtual ~ImageRef() {}
	virtual load_state getState() const = 0;
	virtual size_t getWidth() const = 0;
	virtual size_t getHeight() const = 0;
	virtual void* getNativeHandle() const = 0;
};

struct IJiraPainter {
	struct point { int x; int y; };
	struct size { size_t width; size_t height; };

	virtual ~IJiraPainter() {}
	virtual void moveOrigin(int x, int y) = 0;
	void moveOrigin(const point& pt) { moveOrigin(pt.x, pt.y); }
	virtual point getOrigin() const = 0;
	virtual void setOrigin(const point& orig) = 0;
	virtual void paintImage(const std::string& url, size_t width, size_t height) = 0;
	virtual void paintImage(const ImageRef* img, size_t width, size_t height) = 0;
	virtual void paintString(const std::string& text) = 0;
	virtual size measureString(const std::string& text) = 0;
	virtual StyleSave* setStyle(jira::styles) = 0;
	virtual StyleSave* setStyle(rules) = 0;
	virtual void restoreStyle(StyleSave*) = 0;
};

class PushOrigin {
	IJiraPainter* painter;
	IJiraPainter::point orig;
public:
	explicit PushOrigin(IJiraPainter* painter) : painter(painter), orig(painter->getOrigin())
	{
	}

	~PushOrigin()
	{
		painter->setOrigin(orig);
	}
};

class StyleSaver {
	IJiraPainter* painter;
	StyleSave* save;
public:
	explicit StyleSaver(IJiraPainter* painter, jira::styles style, rules rule) : painter(painter), save(nullptr)
	{
		if (style == jira::styles::unset)
			save = painter->setStyle(rule);
		else
			save = painter->setStyle(style);
	}

	~StyleSaver()
	{
		painter->restoreStyle(save);
	}
};

struct IJiraNode : jira::node {
	using point = IJiraPainter::point;
	using size = IJiraPainter::size;

	virtual std::string text() const = 0;
	virtual jira::styles getStyles() const = 0;
	virtual void paint(IJiraPainter* painter) = 0;
	virtual void measure(IJiraPainter* painter) = 0;
	virtual void setPosition(int x, int y) = 0;
	virtual point getPosition() = 0;
	virtual size getSize() = 0;
	virtual void setClass(rules) = 0;
	virtual rules getRules() const = 0;

	virtual IJiraNode* getParent() const = 0;
	virtual void setParent(IJiraNode*) = 0;
	virtual void invalidate() = 0;
	virtual void invalidate(int x, int y, size_t width, size_t height) = 0;

	virtual jira::node* findHovered(int x, int y) = 0;
};

inline IJiraNode* cast(const std::unique_ptr<jira::node>& node) {
	return static_cast<IJiraNode*>(node.get());
}


class CAppModel : public jira::listeners<CAppModelListener, CAppModel> {
	std::vector<std::shared_ptr<jira::server>> m_servers;
	std::shared_ptr<jira::document> m_document;

	void onListChanged(uint32_t addedOrRemoved);
	std::mutex m_guard;

	static std::shared_ptr<ImageRef> image_creator(const std::shared_ptr<jira::server>& srv, const std::string&);
public:
	CAppModel();

	void startup();

	void lock();
	const std::vector<std::shared_ptr<jira::server>>& servers() const;
	const std::shared_ptr<jira::document>& document() const;
	void unlock();

	void add(const std::shared_ptr<jira::server>& server);
	void remove(const std::shared_ptr<jira::server>& server);
	void update(const std::shared_ptr<jira::server>& server);
};
