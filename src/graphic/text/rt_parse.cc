/*
 * Copyright (C) 2006-2012 by the Widelands Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "graphic/text/rt_parse.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <SDL.h>
#include <boost/format.hpp>

#include "graphic/text/rt_errors_impl.h"
#include "graphic/text/textstream.h"


namespace RT {

Attr::Attr(const std::string& gname, const std::string& value) : m_name(gname), m_value(value) {
}

const std::string& Attr::name() const {
	return m_name;
}

long Attr::get_int() const {
	long rv = strtol(m_value.c_str(), nullptr, 10);
	return rv;
}

std::string Attr::get_string() const {
	return m_value;
}

bool Attr::get_bool() const {
	if (m_value == "true" || m_value == "1" || m_value == "yes")
		return true;
	return false;
}

RGBColor Attr::get_color() const {
	if (m_value.size() != 6)
		throw InvalidColor((boost::format("Could not parse '%s' as a color.") % m_value).str());

	uint32_t clrn = strtol(m_value.c_str(), nullptr, 16);
	return RGBColor((clrn >> 16) & 0xff, (clrn >> 8) & 0xff, clrn & 0xff);
}

void AttrMap::add_attribute(const std::string& name, Attr* a) {
	m_attrs[name] = std::unique_ptr<Attr>(a);
}

const Attr& AttrMap::operator[](const std::string& s) const {
	const auto i = m_attrs.find(s);
	if (i == m_attrs.end()) {
		throw AttributeNotFound(s);
	}
	return *(i->second);
}

bool AttrMap::has(const std::string& s) const {
	return m_attrs.count(s);
}

const std::string& Tag::name() const {
	return m_name;
}

const AttrMap& Tag::attrs() const {
	return m_am;
}

const Tag::ChildList& Tag::childs() const {
	return m_childs;
}

Tag::~Tag() {
	while (m_childs.size()) {
		delete m_childs.back();
		m_childs.pop_back();
	}
}

void Tag::m_parse_opening_tag(TextStream & ts, TagConstraints & tcs) {
	ts.expect("<");
	m_name = ts.till_any(" \t\n>");
	ts.skip_ws();

	while (ts.peek(1) != ">") {
		m_parse_attribute(ts, tcs[m_name].allowed_attrs);
		ts.skip_ws();
	}

	ts.expect(">");
}

void Tag::m_parse_closing_tag(TextStream & ts) {
	ts.expect("</");
	ts.expect(m_name, false);
	ts.expect(">", false);
}

void Tag::m_parse_attribute(TextStream & ts, std::unordered_set<std::string> & allowed_attrs) {
	std::string aname = ts.till_any("=");
	if (!allowed_attrs.count(aname))
		throw SyntaxErrorImpl(ts.line(), ts.col(), "an allowed attribute", aname, ts.peek(100));

	ts.skip(1);

	m_am.add_attribute(aname, new Attr(aname, ts.parse_string()));
}

void Tag::m_parse_content(TextStream & ts, TagConstraints & tcs, const TagSet & allowed_tags)
{
	TagConstraint tc = tcs[m_name];

	for (;;) {
		if (!tc.text_allowed)
			ts.skip_ws();

		size_t line = ts.line(), col = ts.col();
		std::string text = ts.till_any("<");
		if (text != "") {
			if (!tc.text_allowed) {
				throw SyntaxErrorImpl(line, col, "no text, as only tags are allowed here", text, ts.peek(100));
			}
			m_childs.push_back(new Child(text));
		}

		if (ts.peek(2 + m_name.size()) == ("</" + m_name))
			break;

		Tag * child = new Tag();
		line = ts.line(); col = ts.col(); size_t cpos = ts.pos();
		child->parse(ts, tcs, allowed_tags);
		if (!tc.allowed_childs.count(child->name()))
			throw SyntaxErrorImpl(line, col, "an allowed tag", child->name(), ts.peek(100, cpos));
		if (!allowed_tags.empty() && !allowed_tags.count(child->name()))
			throw SyntaxErrorImpl(line, col, "an allowed tag", child->name(), ts.peek(100, cpos));

		m_childs.push_back(new Child(child));
	}
}

void Tag::parse(TextStream & ts, TagConstraints & tcs, const TagSet & allowed_tags) {
	m_parse_opening_tag(ts, tcs);

	TagConstraint tc = tcs[m_name];
	if (tc.has_closing_tag) {
		m_parse_content(ts, tcs, allowed_tags);
		m_parse_closing_tag(ts);
	}
}


/*
 * Class Parser
 */
Parser::Parser() {
	{ // rt tag
		TagConstraint tc;
		tc.allowed_attrs.insert("padding");
		tc.allowed_attrs.insert("padding_r");
		tc.allowed_attrs.insert("padding_l");
		tc.allowed_attrs.insert("padding_b");
		tc.allowed_attrs.insert("padding_t");
		tc.allowed_attrs.insert("db_show_spaces");
		tc.allowed_attrs.insert("keep_spaces"); // Keeps blank spaces intact for text editing
		tc.allowed_attrs.insert("background");

		tc.allowed_childs.insert("p");
		tc.allowed_childs.insert("vspace");
		tc.allowed_childs.insert("font");
		tc.allowed_childs.insert("sub");
		tc.text_allowed = false;
		tc.has_closing_tag = true;
		m_tcs["rt"] = tc;
	}
	{ // br tag
		TagConstraint tc;
		tc.text_allowed = false;
		tc.has_closing_tag = false;
		m_tcs["br"] = tc;
	}
	{ // img tag
		TagConstraint tc;
		tc.allowed_attrs.insert("src");
		tc.allowed_attrs.insert("ref");
		tc.text_allowed = false;
		tc.has_closing_tag = false;
		m_tcs["img"] = tc;
	}
	{ // vspace tag
		TagConstraint tc;
		tc.allowed_attrs.insert("gap");
		tc.text_allowed = false;
		tc.has_closing_tag = false;
		m_tcs["vspace"] = tc;
	}
	{ // space tag
		TagConstraint tc;
		tc.allowed_attrs.insert("gap");
		tc.allowed_attrs.insert("fill");
		tc.text_allowed = false;
		tc.has_closing_tag = false;
		m_tcs["space"] = tc;
	}
	{ // sub tag
		TagConstraint tc;
		tc.allowed_attrs.insert("padding");
		tc.allowed_attrs.insert("padding_r");
		tc.allowed_attrs.insert("padding_l");
		tc.allowed_attrs.insert("padding_b");
		tc.allowed_attrs.insert("padding_t");
		tc.allowed_attrs.insert("margin");
		tc.allowed_attrs.insert("float");
		tc.allowed_attrs.insert("valign");
		tc.allowed_attrs.insert("background");
		tc.allowed_attrs.insert("width");

		tc.allowed_childs.insert("p");
		tc.allowed_childs.insert("vspace");
		tc.allowed_childs.insert("font");
		tc.allowed_childs.insert("sub");

		tc.text_allowed = false;
		tc.has_closing_tag = true;
		m_tcs["sub"] = tc;
	}
	{ // p tag
		TagConstraint tc;
		tc.allowed_attrs.insert("indent");
		tc.allowed_attrs.insert("align");
		tc.allowed_attrs.insert("valign");
		tc.allowed_attrs.insert("spacing");

		tc.allowed_childs.insert("font");
		tc.allowed_childs.insert("space");
		tc.allowed_childs.insert("br");
		tc.allowed_childs.insert("img");
		tc.allowed_childs.insert("sub");
		tc.text_allowed = true;
		tc.has_closing_tag = true;
		m_tcs["p"] = tc;
	}
	{ // font tag
		TagConstraint tc;
		tc.allowed_attrs.insert("size");
		tc.allowed_attrs.insert("face");
		tc.allowed_attrs.insert("color");
		tc.allowed_attrs.insert("bold");
		tc.allowed_attrs.insert("italic");
		tc.allowed_attrs.insert("underline");
		tc.allowed_attrs.insert("shadow");
		tc.allowed_attrs.insert("ref");

		tc.allowed_childs.insert("br");
		tc.allowed_childs.insert("space");
		tc.allowed_childs.insert("p");
		tc.allowed_childs.insert("font");
		tc.allowed_childs.insert("sub");
		tc.text_allowed = true;
		tc.has_closing_tag = true;
		m_tcs["font"] = tc;
	}
}

Parser::~Parser() {
}

Tag * Parser::parse(std::string text, const TagSet & allowed_tags) {
	m_ts.reset(new TextStream(text));

	m_ts->skip_ws(); m_ts->rskip_ws();
	Tag * rv = new Tag();
	rv->parse(*m_ts, m_tcs, allowed_tags);

	return rv;
}
std::string Parser::remaining_text() {
	if (m_ts == nullptr)
		return "";
	return m_ts->remaining_text();
}

}
