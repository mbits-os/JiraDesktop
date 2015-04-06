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

#include <gui/node.hpp>

#include <ratio>
#include <string>
#include <map>

namespace styles {
	using colorref = uint32_t;

	enum class pseudo {
		unspecified,
		hover,
		active
	};

	enum class align {
		left,
		right,
		center
	};

	enum class line {
		none,
		solid,
		dot,
		dash
	};

	enum class weight {
		normal,
		bold,
		bolder,
		lighter,
		w100 = 100,
		w200 = 200,
		w300 = 300,
		w400 = 400,
		w500 = 500,
		w600 = 600,
		w700 = 700,
		w800 = 800,
		w900 = 900
	};

	template <typename Ratio>
	class length {
		long double m_len;
	public:
		using ratio = Ratio;
		length() : m_len(0.0) {};
		length(long double v) : m_len(v) {};

		long double value() const { return m_len; }
	};

	using pixels = length<std::ratio<1>>;
	using points = length<std::ratio<4, 3>>;
	using inches = length<std::ratio<96>>;

	template<class To, class Ratio>
	inline To length_cast(const length<Ratio>& size)
	{
		typedef std::ratio_divide<Ratio, typename To::ratio> _CF;

		if (_CF::num == 1 && _CF::den == 1)
			return size.value();

		if (_CF::num != 1 && _CF::den == 1)
			return size.value() * _CF::num;
		
		if (_CF::num == 1 && _CF::den != 1)
			return size.value() / _CF::den;

		return return size.value() * _CF::num / _CF::den;
	}

	class ems {
		long double m_len;
	public:
		ems() : m_len(0.0) {};
		ems(long double v) : m_len(v) {}

		long double value() const { return m_len; }
		template <typename Ratio>
		length<Ratio> value(const length<Ratio>& len) const {
			return len.value() * m_len;
		}
	};

	template <typename T1, typename T2>
	struct union_t
	{
		enum class which { neither, first, second };

		union_t() : set(which::neither), deleter(nullptr), copier(nullptr) {}
		explicit union_t(const T1& arg)
			: set(which::first)
			, deleter(del<T1>)
			, copier(copy<T1>)
		{
			copier(ptr, &arg);
		}
		explicit union_t(const T2& arg)
			: set(which::second)
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
			u.set = which::neither;
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
	private:
		which set;
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

	inline namespace literals {
		inline pixels operator""_px(uint64_t v) { return (long double)v; }
		inline ems operator""_em(uint64_t v) { return (long double)v; }
		inline points operator""_pt(uint64_t v) { return (long double)v; }
		inline inches operator""_in(uint64_t v) { return (long double)v; }

		inline pixels operator""_px(long double v) { return v; }
		inline ems operator""_em(long double v) { return v; }
		inline points operator""_pt(long double v) { return v; }
		inline inches operator""_in(long double v) { return v; }
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
		prop_border_color,
	};
	template <> struct prop_traits<color_prop> : prop_traits_impl<colorref> {};

	enum bool_prop {
		prop_italic,
		prop_underline,
	};
	template <> struct prop_traits<bool_prop> : prop_traits_impl<bool>{};

	enum string_prop {
		prop_font_family
	};
	template <> struct prop_traits<string_prop> : prop_traits_impl<std::string, const std::string&>{};

	enum length_prop {
		prop_border_length,
		prop_font_size
	};
	template <> struct prop_traits<length_prop> : prop_traits_impl<pixels, const pixels&>{};

	enum rel_length_prop {
		prop_font_size_em
	};
	template <> struct prop_traits<rel_length_prop> : prop_traits_impl<ems, const ems&>{};

#define ENUM_PROP(name_base, type) \
	enum name_base ## _prop { prop_ ## name_base }; \
	template <> struct prop_traits<name_base ## _prop> : prop_traits_impl<type>{};

	ENUM_PROP(font_weight, weight);
	ENUM_PROP(text_align, align);
	ENUM_PROP(border_style, line);

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
		STORAGE(rel_length)
		STORAGE(font_weight)
		STORAGE(text_align)
		STORAGE(border_style)

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
			m_rel_lengths.merge(rhs.m_rel_lengths);
			m_font_weights.merge(rhs.m_font_weights);
			m_text_aligns.merge(rhs.m_text_aligns);
			m_border_styles.merge(rhs.m_border_styles);

			if (rhs.has(prop_font_size)) {
				auto it = m_rel_lengths.m_items.find(prop_font_size_em);
				if (it != m_rel_lengths.m_items.end())
					m_rel_lengths.m_items.erase(it);
			}

			if (rhs.has(prop_font_size_em)) {
				auto it = m_lengths.m_items.find(prop_font_size);
				if (it != m_lengths.m_items.end())
					m_lengths.m_items.erase(it);
			}
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
				&& m_rel_lengths.empty()
				&& m_font_weights.empty()
				&& m_text_aligns.empty()
				&& m_border_styles.empty();
		}
	};

	inline rule_storage operator<< (const rule_storage& lhs, const rule_storage& rhs)
	{
		rule_storage copy = lhs;
		return std::move(copy <<= rhs);
	};

	template <typename Prop>
	inline rule_storage rule(Prop prop, prop_arg_t<Prop> value) { return rule_storage{}.set(prop, std::forward<prop_arg_t<Prop>>(value)); }
	inline rule_storage color(colorref color) { return rule(prop_color, color); }
	inline rule_storage background(colorref color) { return rule(prop_background, color); }
	inline rule_storage italic(bool value = true) { return rule(prop_italic, value); }
	inline rule_storage underline(bool value = true) { return rule(prop_underline, value); }
	inline rule_storage textAlign(align a) { return rule(prop_text_align, a); }
	inline rule_storage fontSize(ems em) { return rule(prop_font_size_em, em); }
	inline rule_storage fontSize(pixels px) { return rule(prop_font_size, px); }
	template <typename Ratio>
	inline rule_storage fontSize(length<Ratio> size) { return fontSize(length_cast<pixels>(size)); }
	inline rule_storage fontWeight(weight w) { return rule(prop_font_weight, w); }
	inline rule_storage fontFamily(const std::string& face) { return rule(prop_font_family, face); }
	inline rule_storage border(line style, pixels length, colorref color)
	{
		return rule(prop_border_style, style)
			.set(prop_border_length, length)
			.set(prop_border_color, color);
	}
	template <typename Ratio>
	inline rule_storage border(line type, length<Ratio> size, colorref color) { return border(type, length_cast<pixels>(size), color); }

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

		bool selects(const gui::node* node) const
		{
			if (!maySelect(node))
				return false;

			switch (m_pseudoClass) {
			case pseudo::hover:
				if (!node->getHovered())
					return false;
				break;
			case pseudo::active:
				if (!node->getActive())
					return false;
				break;
			};

			return true;
		}

		// same as selects, but doesn't take pseudo classes into account
		bool maySelect(const gui::node* node) const
		{
			if (!node)
				return false;

			if (m_elemName != gui::elem::unspecified && m_elemName != node->getNodeName())
				return false;

			for (auto& cl : m_classes) {
				if (!node->hasClass(cl))
					return false;
			}

			return true;
		}
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