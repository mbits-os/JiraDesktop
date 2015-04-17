#pragma once

#include <gui/node.hpp>
#include <atomic>

namespace gui {
	enum class Attr {
		Id,
		Text,
		Href,
		Tooltip
	};

	class node_base : public node, public std::enable_shared_from_this<node> {
	public:
		node_base(elem name);
		node_base(const node_base&);
		const std::string& getId() const override;
		void setId(const std::string& id) override;
		elem getNodeName() const override;
		void addClass(const std::string& name) override;
		void removeClass(const std::string& name) override;
		bool hasClass(const std::string& name) const override;
		const std::vector<std::string>& getClassNames() const override;
		std::string text() const override;
		void setTooltip(const std::string& text) override;
		std::shared_ptr<node> insertBefore(const std::shared_ptr<node>& newChild, const std::shared_ptr<node>& refChild) override;
		std::shared_ptr<node> replaceChild(const std::shared_ptr<node>& newChild, const std::shared_ptr<node>& oldChild) override;
		std::shared_ptr<node> removeChild(const std::shared_ptr<node>& oldChild) override;
		std::shared_ptr<node> appendChild(const std::shared_ptr<node>& newChild) override;
		bool hasChildNodes() const override;
		std::shared_ptr<node> cloneNode(bool deep) const override;
		const std::vector<std::shared_ptr<node>>& children() const override;

		void paint(painter* painter) override final;
		void measure(painter* painter) override final;
		void setPosition(const pixels& x, const pixels& y) override;
		void setSize(const pixels& width, const pixels& height) override;
		point getPosition() override;
		point getAbsolutePos() override;
		size getSize() override;
		pixels getBaseline() override;

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
		virtual bool isSupported(const std::shared_ptr<node>&);
		virtual void onAdded(const std::shared_ptr<node>&);
		virtual void onRemoved(const std::shared_ptr<node>&);
		virtual std::shared_ptr<node> cloneSelf() const = 0;

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
		pixels m_baseline;

		std::atomic<int> m_hoverCount{ 0 };
		std::atomic<int> m_activeCount{ 0 };

		std::shared_ptr<styles::rule_storage> m_calculated;
		std::shared_ptr<styles::rule_storage> m_calculatedHover;
		std::shared_ptr<styles::rule_storage> m_calculatedActive;
		std::shared_ptr<styles::rule_storage> m_calculatedHoverActive;
		std::shared_ptr<styles::stylesheet> m_allApplying;

		inline static std::shared_ptr<node> cloneDetach(const std::shared_ptr<node>& node) {
			node->setParent({});
			return node;
		}
	private:
		bool imChildOf(const std::shared_ptr<node>&) const;
	};
}