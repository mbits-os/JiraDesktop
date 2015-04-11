#pragma once

#include <gui/node.hpp>
#include <gui/document.hpp>
#include <gui/styles.hpp>
#include <atomic>
#include <gui/image.hpp>

namespace gui {
	enum class Attr {
		Text,
		Href,
		Tooltip
	};

	class node_base : public node, public std::enable_shared_from_this<node> {
	public:
		node_base(elem name);
		elem getNodeName() const override;
		void addClass(const std::string& name) override;
		void removeClass(const std::string& name) override;
		bool hasClass(const std::string& name) const override;
		const std::vector<std::string>& getClassNames() const override;
		std::string text() const override;
		void setTooltip(const std::string& text) override;
		void addChild(const std::shared_ptr<node>& child) override;
		const std::vector<std::shared_ptr<node>>& children() const override;

		void paint(painter* painter) override final;
		void measure(painter* painter) override final;
		void setPosition(const pixels& x, const pixels& y) override;
		void setSize(const pixels& width, const pixels& height) override;
		point getPosition() override;
		point getAbsolutePos() override;
		size getSize() override;

		std::shared_ptr<node> getParent() const override;
		void setParent(const std::shared_ptr<node>&) override;
		void invalidate() override;
		void invalidate(const point& pt, const size& size) override;

		std::shared_ptr<node> nodeFromPoint(const pixels& x, const pixels& y) override;
		void setHovered(bool hovered) override;
		bool getHovered() const override;
		void setActive(bool active) override;
		bool getActive() const override;
		void activate() override;
		pointer getCursor() const override;

		bool hasTooltip() const override;
		const std::string& getTooltip() const override;

		void innerText(const std::string& text) override;

		std::shared_ptr<styles::rule_storage> calculatedStyle() const override;
		std::shared_ptr<styles::rule_storage> normalCalculatedStyles() const override;
		std::shared_ptr<styles::stylesheet> styles() const override;
		void applyStyles(const std::shared_ptr<styles::stylesheet>& stylesheet) override;
		void calculateStyles();
		pixels offsetLeft() const; // border-left-width + padding-left
		pixels offsetTop() const; // border-top-width + padding-top
		pixels offsetRight() const; // border-right-width + padding-right
		pixels offsetBottom() const; // border-bottom-width + padding-bottom

		void openLink(const std::string& url);
		virtual void paintContents(painter* painter,
			const pixels& offX, const pixels& offY);
		virtual size measureContents(painter* painter,
			const pixels& offX, const pixels& offY) = 0;

	protected:
		elem m_nodeName;
		std::map<Attr, std::string> m_data;
		std::vector<std::shared_ptr<node>> m_children;
		std::vector<std::string> m_classes;
		std::weak_ptr<node> m_parent;
		struct {
			point pt;
			size size;
		} m_position;

		std::atomic<int> m_hoverCount{ 0 };
		std::atomic<int> m_activeCount{ 0 };

		std::shared_ptr<styles::rule_storage> m_calculated;
		std::shared_ptr<styles::rule_storage> m_calculatedHover;
		std::shared_ptr<styles::rule_storage> m_calculatedActive;
		std::shared_ptr<styles::rule_storage> m_calculatedHoverActive;
		std::shared_ptr<styles::stylesheet> m_allApplying;
	};

	class image_cb
		: public image_ref_callback
		, public std::enable_shared_from_this<image_cb> {

	public:
		std::weak_ptr<node> parent;
		void onImageChange(image_ref*) override;
	};

	class icon_node : public node_base {
		std::shared_ptr<image_ref> m_image;
		std::shared_ptr<image_cb> m_cb;
	public:
		icon_node(const std::string& uri, const std::shared_ptr<image_ref>& image, const std::string& tooltip);
		~icon_node();
		void attach();
		void addChild(const std::shared_ptr<node>& child) override;
		void paintContents(painter* painter,
			const pixels& offX, const pixels& offY) override;
		size measureContents(painter* painter,
			const pixels& offX, const pixels& offY) override;
	};

	class document_base;
	class user_node : public node_base {
		std::weak_ptr<document_base> m_document;
		std::shared_ptr<image_ref> m_image;
		std::shared_ptr<image_cb> m_cb;
		std::map<uint32_t, std::string> m_urls;
		int m_selectedSize;
	public:
		user_node(const std::weak_ptr<document_base>& document, std::map<uint32_t, std::string>&& avatar, const std::string& tooltip);
		~user_node();
		void addChild(const std::shared_ptr<node>& child) override;
		void paintContents(painter* painter,
			const pixels& offX, const pixels& offY) override;
		size measureContents(painter* painter,
			const pixels& offX, const pixels& offY) override;
	};

	class block_node : public node_base {
	public:
		block_node(elem name);
		size measureContents(painter* painter,
			const pixels& offX, const pixels& offY) override;
	};

	class span_node : public node_base {
	public:
		span_node(elem name);
		size measureContents(painter* painter,
			const pixels& offX, const pixels& offY) override;
	};

	class CJiraLinkNode : public span_node {
	public:
		CJiraLinkNode(const std::string& href);
	};

	class CJiraTextNode : public node_base {
	public:
		CJiraTextNode(const std::string& text);
		void paintContents(painter* painter,
			const pixels& offX, const pixels& offY) override;
		size measureContents(painter* painter,
			const pixels& offX, const pixels& offY) override;
	};

	struct image_creator {
		virtual ~image_creator() {}
		virtual std::shared_ptr<image_ref> create(const std::string&) = 0;
	};

	class document_base : public document, public std::enable_shared_from_this<document_base> {
		std::shared_ptr<node> createIcon(const std::string& uri, const std::string& text, const std::string& description) override;
		std::shared_ptr<node> createUser(bool active, const std::string& display, const std::string& email, const std::string& login, std::map<uint32_t, std::string>&& avatar) override;
		std::shared_ptr<node> createLink(const std::string& href) override;
		std::shared_ptr<node> createText(const std::string& text) override;
		std::shared_ptr<node> createElement(const elem name) override;

		std::mutex m_guard;
		std::map<std::string, std::shared_ptr<image_ref>> m_cache;
		std::shared_ptr<image_creator> m_creator;

	public:
		explicit document_base(const std::shared_ptr<image_creator>&);

		std::shared_ptr<image_ref> createImage(const std::string& uri);
	};

	class table_node : public block_node {
		std::shared_ptr<std::vector<pixels>> m_columns;
	public:
		table_node();

		void addChild(const std::shared_ptr<node>& child) override final;
		size measureContents(painter* painter,
			const pixels& offX, const pixels& offY) override;
	};

	class row_node : public span_node {
	protected:
		std::shared_ptr<std::vector<pixels>> m_columns;
	public:
		row_node(elem name);
		void setColumns(const std::shared_ptr<std::vector<pixels>>& columns);

		size measureContents(painter* painter,
			const pixels& offX, const pixels& offY) override;

		virtual void repositionChildren();
	};

	class caption_row_node : public row_node {
	public:
		caption_row_node();

		size measureContents(painter* painter,
			const pixels& offX, const pixels& offY) override;

		void repositionChildren() override;
	};

	class doc_element : public block_node {
		std::function<void(const point&, const size&)> m_invalidator;
	public:
		explicit doc_element(const std::function<void(const point&, const size&)>& invalidator);
		void invalidate(const point& pt, const size& size) override;
	};
}