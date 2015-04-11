#pragma once

#include <gui/node.hpp>
#include <atomic>

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
}