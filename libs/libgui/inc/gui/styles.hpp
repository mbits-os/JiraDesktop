/*
 * Copyright (C) 2015 midnightBITS
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <gui/types.hpp>
#include <ratio>
#include <string>
#include <map>
#include <vector>
#include <memory>

namespace gui {
	struct node;
}

namespace styles {

	enum class pseudo {
		unspecified,
		hover,
		active
	};

	class ems {
		long double m_len;
	public:
		ems() : m_len(0.0) {};
		ems(long double v) : m_len(v) {}

		long double value() const { return m_len; }
		template <typename Ratio>
		gui::length<Ratio> value(const gui::length<Ratio>& len) const {
			return len.value() * m_len;
		}
	};

	inline namespace literals {
		inline ems operator""_em(uint64_t v) { return (long double)v; }
		inline ems operator""_em(long double v) { return v; }
	};

	template <typename T1, typename T2>
	struct union_t
	{
		enum which_t { neither_type, first_type, second_type };

		union_t() : set(which_t::neither_type), deleter(nullptr), copier(nullptr) {}
		union_t(const T1& arg)
			: set(which_t::first_type)
			, deleter(del<T1>)
			, copier(copy<T1>)
		{
			copier(ptr, &arg);
		}
		union_t(const T2& arg)
			: set(which_t::second_type)
			, deleter(del<T2>)
			, copier(copy<T2>)
		{
			copier(ptr, &arg);
		}

		union_t(const union_t& u)
			: set(u.set)
			, deleter(u.deleter)
			, copier(u.copier)
		{
			if (copier)
				copier(ptr, u.ptr);
		}

		union_t& operator=(const union_t& u)
		{
			if (this == &u)
				return *this;

			void* tmp = nullptr;
			if (u.copier)
				u.copier(tmp, u.ptr);

			if (deleter)
				deleter(ptr);
			ptr = tmp;

			set = u.set;
			copier = u.copier;
			deleter = u.deleter;

			return *this;
		}

		union_t(union_t&& u)
			: set(u.set)
			, deleter(u.deleter)
			, copier(u.copier)
			, ptr(u.ptr)
		{
			u.set = which_t::neither_type;
			u.copier = nullptr;
			u.deleter = nullptr;
			u.ptr = nullptr;
		}

		union_t& operator=(union_t&& u)
		{
			std::swap(set, u.set);
			std::swap(ptr, u.ptr);
			std::swap(copier, u.copier);
			std::swap(deleter, u.deleter);

			return *this;
		}

		~union_t()
		{
			if (deleter)
				deleter(ptr);
		}

		which_t which() const { return set; }
		const T1& first() const { return *reinterpret_cast<const T1*>(ptr); }
		const T2& second() const { return *reinterpret_cast<const T2*>(ptr); }
	private:
		which_t set;
		void* ptr = nullptr;
		void(*deleter)(void*& ptr);
		void(*copier)(void*& lhs, const void* rhs);

		template <typename T>
		static void del(void*& ptr)
		{
			delete reinterpret_cast<T*>(ptr);
			ptr = nullptr;
		}

		template <typename T>
		static void copy(void*& lhs, const void* rhs)
		{
			lhs = new T(*reinterpret_cast<const T*>(rhs));
		}
	};

	template <typename T> struct prop_traits;
	template <typename T> using prop_storage_t = typename prop_traits<T>::storage_t;
	template <typename T> using prop_arg_t = typename prop_traits<T>::arg_t;

	template <typename Storage, typename Arg = Storage>
	struct prop_traits_impl
	{
		using storage_t = Storage;
		using arg_t = Arg;
	};

	enum color_prop {
		prop_color,
		prop_background,
		prop_border_top_color,
		prop_border_right_color,
		prop_border_bottom_color,
		prop_border_left_color,
	};
	template <> struct prop_traits<color_prop> : prop_traits_impl<gui::colorref>{};

	enum bool_prop {
		prop_italic,
		prop_underline,
		prop_visibility
	};
	template <> struct prop_traits<bool_prop> : prop_traits_impl<bool>{};

	enum string_prop {
		prop_font_family
	};
	template <> struct prop_traits<string_prop> : prop_traits_impl<std::string, const std::string&>{};

	using length_u = union_t<gui::pixels, ems>;
	enum length_prop {
		prop_font_size,
		prop_border_top_width,
		prop_border_right_width,
		prop_border_bottom_width,
		prop_border_left_width,
		prop_padding_top,
		prop_padding_right,
		prop_padding_bottom,
		prop_padding_left,
		prop_margin_top,
		prop_margin_right,
		prop_margin_bottom,
		prop_margin_left
	};
	template <> struct prop_traits<length_prop> : prop_traits_impl<length_u, const length_u&>{};

	enum border_style_prop {
		prop_border_top_style,
		prop_border_right_style,
		prop_border_bottom_style,
		prop_border_left_style
	};
	template <> struct prop_traits<border_style_prop> : prop_traits_impl<gui::line_style>{};

#define ENUM_PROP(name_base, type) \
	enum name_base ## _prop { prop_ ## name_base }; \
	template <> struct prop_traits<name_base ## _prop> : prop_traits_impl<type>{};

	ENUM_PROP(font_weight, gui::weight);
	ENUM_PROP(text_align, gui::align);
	ENUM_PROP(cursor, gui::pointer);
	ENUM_PROP(display, gui::display);

	template <typename Prop>
	struct storage {
		std::map<Prop, prop_storage_t<Prop>> m_items;
		void set(Prop prop, prop_arg_t<Prop> arg)
		{
			auto it = m_items.lower_bound(prop);
			if (it == m_items.end() || it->first != prop)
				m_items.insert(it, std::make_pair(prop, arg));
			else
				it->second = arg;
		}

		bool has(Prop prop) const
		{
			return m_items.find(prop) != m_items.end();
		}

		prop_storage_t<Prop> get(Prop prop) const
		{
			auto it = m_items.find(prop);
			if (it != m_items.end())
				return it->second;

			return{};
		}

		void merge(const storage<Prop>& rhs) {
			for (auto& pair : rhs.m_items) {
				auto it = m_items.lower_bound(pair.first);
				if (it == m_items.end() || it->first != pair.first)
					m_items.insert(it, pair);
				else
					it->second = pair.second;
			}
		}

		bool empty() const
		{
			return m_items.empty();
		}
	};

	struct rule_storage {
#define STORAGE(TYPE) \
	storage<TYPE ## _prop> m_ ## TYPE ## s; \
	rule_storage& set(TYPE ## _prop prop, prop_arg_t<TYPE ## _prop> arg) { m_ ## TYPE ## s.set(prop, std::forward<prop_arg_t<TYPE ## _prop>>(arg)); return *this; } \
	bool has(TYPE ## _prop prop) const { return m_ ## TYPE ## s.has(prop); } \
	prop_storage_t<TYPE ## _prop> get(TYPE ## _prop prop) const { return m_ ## TYPE ## s.get(prop); }

		STORAGE(color)
		STORAGE(bool)
		STORAGE(string)
		STORAGE(length)
		STORAGE(font_weight)
		STORAGE(text_align)
		STORAGE(border_style)
		STORAGE(cursor)
		STORAGE(display)

		template <typename key, typename val>
		void merge(std::map<key, val>& lhs, const std::map<key, val>& rhs) {
			for (auto& pair : rhs) {
				auto it = lhs.lower_bound(pair.first);
				if (it == lhs.end() || it->first != pair.first)
					lhs.insert(it, pair);
				else
					it->second = pair.second;
			}
		}

		void merge(const rule_storage& rhs) {
			m_colors.merge(rhs.m_colors);
			m_bools.merge(rhs.m_bools);
			m_strings.merge(rhs.m_strings);
			m_lengths.merge(rhs.m_lengths);
			m_font_weights.merge(rhs.m_font_weights);
			m_text_aligns.merge(rhs.m_text_aligns);
			m_border_styles.merge(rhs.m_border_styles);
			m_cursors.merge(rhs.m_cursors);
			m_displays.merge(rhs.m_displays);
		}

		rule_storage& operator<<= (const rule_storage& rhs)
		{
			merge(rhs);
			return *this;
		};

		bool empty() const
		{
			return m_colors.empty()
				&& m_bools.empty()
				&& m_strings.empty()
				&& m_lengths.empty()
				&& m_font_weights.empty()
				&& m_text_aligns.empty()
				&& m_border_styles.empty()
				&& m_cursors.empty()
				&& m_displays.empty();
		}
	};

	inline rule_storage operator<< (const rule_storage& lhs, const rule_storage& rhs)
	{
		rule_storage copy = lhs;
		return std::move(copy <<= rhs);
	};

#define MAKE_FOURWAY(x) \
	x(top) \
	x(right) \
	x(bottom) \
	x(left)

#define LENGTH_PROP(creator, prop) \
	inline rule_storage creator(const ems& em) { return rule(prop, em); } \
	inline rule_storage creator(const gui::pixels& px) { return rule(prop, px); } \
	template <typename Ratio> inline rule_storage creator(const gui::length<Ratio>& size) { return creator(gui::length_cast<pixels>(size)); }

	namespace def {
		template <typename Prop>
		inline rule_storage rule(Prop prop, prop_arg_t<Prop> value) { return rule_storage{}.set(prop, std::forward<prop_arg_t<Prop>>(value)); }
		inline rule_storage color(gui::colorref color) { return rule(prop_color, color); }
		inline rule_storage background(gui::colorref color) { return rule(prop_background, color); }
		inline rule_storage italic(bool value = true) { return rule(prop_italic, value); }
		inline rule_storage underline(bool value = true) { return rule(prop_underline, value); }
		inline rule_storage visible(bool value = true) { return rule(prop_visibility, value); }
		inline rule_storage hidden() { return visible(false); }
		inline rule_storage text_align(gui::align a) { return rule(prop_text_align, a); }
		LENGTH_PROP(font_size, prop_font_size)
		inline rule_storage font_weight(gui::weight w) { return rule(prop_font_weight, w); }
		inline rule_storage font_family(const std::string& face) { return rule(prop_font_family, face); }
		inline rule_storage cursor(gui::pointer value) { return rule(prop_cursor, value); }
		inline rule_storage display(gui::display value) { return rule(prop_display, value); }

#define BORDER_STYLE(side) \
	inline rule_storage border_ ## side ## _style(gui::line_style style) { return rule(prop_border_ ## side ## _style, style); }
		MAKE_FOURWAY(BORDER_STYLE)
#undef BORDER_STYLE

		inline rule_storage border_style(gui::line_style style)
		{
			return border_top_style(style) << border_right_style(style) << border_bottom_style(style) << border_left_style(style);
		}

#define BORDER_COLOR(side) \
	inline rule_storage border_ ## side ## _color(gui::colorref color) { return rule(prop_border_ ## side ## _color, color); }
		MAKE_FOURWAY(BORDER_COLOR)
#undef BORDER_STYLE

		inline rule_storage border_color(gui::colorref color)
		{
			return border_top_color(color) << border_right_color(color) << border_bottom_color(color) << border_left_color(color);
		}

#define BORDER_WIDTH(side) LENGTH_PROP(border_ ## side ## _width, prop_border_ ## side ## _width)
		MAKE_FOURWAY(BORDER_WIDTH)
#undef BORDER_WIDTH

		template <typename Length>
		inline rule_storage border_width(const Length& width)
		{
			return border_top_width(width) << border_right_width(width) << border_bottom_width(width) << border_left_width(width);
		}

#define BORDER(side) \
	template <typename Length> \
	inline rule_storage border_ ## side(const Length& width, gui::line_style style, gui::colorref color) \
	{ \
		return border_ ## side ## _width(width) << border_ ## side ## _style(style) << border_ ## side ## _color(color); \
	}
		MAKE_FOURWAY(BORDER)
#undef BORDER

			template <typename Length>
		inline rule_storage border(const Length& width, gui::line_style style, gui::colorref color)
		{
			return border_width(width) << border_style(style) << border_color(color);
		}

#define PADDING(side) LENGTH_PROP(padding_ ## side, prop_padding_ ## side)
		MAKE_FOURWAY(PADDING)
#undef PADDING

		template <typename Top>
		inline rule_storage padding(const Top& top)
		{
			return padding_left(top) << padding_top(top) << padding_right(top) << padding_bottom(top);
		}

		template <typename Top, typename Right>
		inline rule_storage padding(const Top& top, const Right& right)
		{
			return padding_left(right) << padding_top(top) << padding_right(right) << padding_bottom(top);
		}

		template <typename Top, typename Right, typename Bottom>
		inline rule_storage padding(const Top& top, const Right& right, const Bottom& bottom)
		{
			return padding_left(right) << padding_top(top) << padding_right(right) << padding_bottom(bottom);
		}

		template <typename Top, typename Right, typename Bottom, typename Left>
		inline rule_storage padding(const Top& top, const Right& right, const Bottom& bottom, const Left& left)
		{
			return padding_left(left) << padding_top(top) << padding_right(right) << padding_bottom(bottom);
		}

#define MARGIN(side) LENGTH_PROP(margin_ ## side, prop_margin_ ## side)
		MAKE_FOURWAY(MARGIN)
#undef MARGIN

		template <typename Top>
		inline rule_storage margin(const Top& top)
		{
			return margin_left(top) << margin_top(top) << margin_right(top) << margin_bottom(top);
		}

		template <typename Top, typename Right>
		inline rule_storage margin(const Top& top, const Right& right)
		{
			return margin_left(right) << margin_top(top) << margin_right(right) << margin_bottom(top);
		}

		template <typename Top, typename Right, typename Bottom>
		inline rule_storage margin(const Top& top, const Right& right, const Bottom& bottom)
		{
			return margin_left(right) << margin_top(top) << margin_right(right) << margin_bottom(bottom);
		}

		template <typename Top, typename Right, typename Bottom, typename Left>
		inline rule_storage margin(const Top& top, const Right& right, const Bottom& bottom, const Left& left)
		{
			return margin_left(left) << margin_top(top) << margin_right(right) << margin_bottom(bottom);
		}
	}

	struct selector {
		selector() = default;
		selector(gui::elem elemName) : m_elemName(elemName) {}
		selector(gui::elem elemName, pseudo pseudoClass) : m_elemName(elemName), m_pseudoClass(pseudoClass) {}
		selector(pseudo pseudoClass) : m_pseudoClass(pseudoClass) {}

		selector& addClass(const std::string& name)
		{
			m_classes.push_back(name);
			return *this;
		}

		gui::elem m_elemName = gui::elem::unspecified;
		pseudo m_pseudoClass = pseudo::unspecified;
		std::vector<std::string> m_classes;

		bool selects(const gui::node* node) const;

		// same as selects, but doesn't take pseudo classes into account
		bool maySelect(const gui::node* node) const;
	};

	struct class_name : selector {
		class_name() = delete;
		class_name(const std::string& name) { addClass(name); }
		class_name(const std::string& name, pseudo pseudoClass) : selector{ pseudoClass } { addClass(name); }
		class_name(gui::elem elemName, const std::string& name, pseudo pseudoClass) : selector{ elemName, pseudoClass } { addClass(name); }
		class_name(gui::elem elemName, const std::string& name) : selector{ elemName } { addClass(name); }
		class_name(const char* name) { addClass(name); }
		class_name(const char* name, pseudo pseudoClass) : selector{ pseudoClass } { addClass(name); }
		class_name(gui::elem elemName, const char* name, pseudo pseudoClass) : selector{ elemName, pseudoClass } { addClass(name); }
		class_name(gui::elem elemName, const char* name) : selector{ elemName } { addClass(name); }
	};

	struct ruleset : rule_storage {
		selector m_sel;
		ruleset(const selector& sel, const rule_storage& rules)
			: rule_storage(rules)
			, m_sel(sel)
		{}
	};

	struct stylesheet {
		std::vector<std::shared_ptr<ruleset>> m_rules;
		stylesheet& add(const selector& sel, const rule_storage& rules)
		{
			m_rules.emplace_back(std::make_shared<ruleset>(std::forward<const selector&>(sel), std::forward<const rule_storage&>(rules)));
			return *this;
		}
	};
};
