/*
 * Copyright (C) 2002-2011 by the Widelands Development Team
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/*
 * The original version of the word wrapping algorithm was taken
 * from Wesnoth -- http://www.wesnoth.org
 */

#include "graphic/wordwrap.h"

#include <boost/format.hpp>
#include <unicode/uchar.h>
#include <unicode/unistr.h>

#include "base/log.h"
#include "graphic/font_handler1.h"
#include "graphic/graphic.h"
#include "graphic/rendertarget.h"
#include "graphic/text/bidi.h"

namespace UI {

/**
 * Initialize the wordwrap object with an unlimited line length
 * and a default-constructed text style.
 */
WordWrap::WordWrap() :
	m_wrapwidth(std::numeric_limits<uint32_t>::max()), m_draw_caret(false)
{
}

WordWrap::WordWrap(const TextStyle & style, uint32_t gwrapwidth) :
	m_style(style), m_draw_caret(false)
{
	m_wrapwidth = gwrapwidth;

	if (m_wrapwidth < std::numeric_limits<uint32_t>::max()) {
		if (m_wrapwidth < 2 * LINE_MARGIN)
			m_wrapwidth = 0;
		else
			m_wrapwidth -= 2 * LINE_MARGIN;
	}
}

/**
 * Set the text style for future wrapping operations.
 */
void WordWrap::set_style(const TextStyle & style)
{
	m_style = style;
}

/**
 * Set the wrap width (i.e. line width limit in pixels) for future wrapping operations.
 */
void WordWrap::set_wrapwidth(uint32_t gwrapwidth)
{
	m_wrapwidth = gwrapwidth;
}

/**
 * Return the wrap width. This can be larger than the actual @ref width of the final
 * text.
 */
uint32_t WordWrap::wrapwidth() const
{
	return m_wrapwidth;
}

/**
 * Perform the wrapping computations for the given text and fill in
 * the private data containing the wrapped results.
 */
void WordWrap::wrap(const std::string & text)
{
	m_lines.clear();

	std::string::size_type line_start = 0;
	uint32_t margin =  m_style.calc_width_for_wrapping(0x2003); // Em space

	while (line_start <= text.size()) {
		std::string::size_type next_line_start;
		std::string::size_type line_end;

		compute_end_of_line(text, line_start, line_end, next_line_start, margin);

		LineData ld;
		ld.start = line_start;
		ld.text = text.substr(line_start, line_end - line_start);
		m_lines.push_back(ld);

		line_start = next_line_start;
	}
}


/**
 * Compute the position where the line that starts at \p line_start
 * ends.
 */
void WordWrap::compute_end_of_line
	(const std::string & text,
	 std::string::size_type line_start,
	 std::string::size_type & line_end,
	 std::string::size_type & next_line_start,
	 uint32_t safety_margin)
{
	std::string::size_type minimum_chars = 1; // So we won't get empty lines
	assert(text.empty() || m_wrapwidth > safety_margin);

	std::string::size_type orig_end = text.find('\n', line_start);
	if (orig_end == std::string::npos)
		orig_end = text.size();

	if (m_wrapwidth == std::numeric_limits<uint32_t>::max() || orig_end - line_start <= minimum_chars) {
		// Special fast path when wrapping is disabled or
		// original text line contains at most minimum_chars characters
		line_end = orig_end;
		next_line_start = orig_end + 1;
		return;
	}


	// Optimism: perhaps the entire line fits?
	if (text_width(text.substr(line_start, orig_end - line_start), m_style.font->size())
		 <= m_wrapwidth - safety_margin) {
		line_end = orig_end;
		next_line_start = orig_end + 1;
		return;
	}

	// Okay, we really do need to wrap; get a first rough estimate using binary search
	// Invariant: [line_start, end_lower) fits,
	// but everything strictly longer than [line_start, end_upper) does not fit
	// We force the lower bound to allow at least one character,
	// otherwise the word wrap might get into an infinite loop.
	std::string::size_type end_upper = orig_end - 1;
	std::string::size_type end_lower = line_start + 1;

	while (end_upper - end_lower > 4) {
		std::string::size_type mid = end_lower + (end_upper - end_lower + 1) / 2;

		if (line_fits(i18n::make_ligatures(text.substr(line_start, mid - line_start).c_str()), safety_margin)) {
			end_lower = mid;
		} else {
			end_upper = mid - 1;
		}
	}

	// Narrow it down to a word break
	// Invariant: space points to a space character such that [line_start, space) fits
	std::string::size_type space = end_lower;

	while (space > line_start && text[space] != ' ')
		--space;

	for (;;) {
		std::string::size_type nextspace = text.find(' ', space + 1);
		if (nextspace > end_upper)
			break; // we already know that this cannot possibly fit

		// check whether the next word still fits
		if (!line_fits(i18n::make_ligatures(text.substr(line_start, nextspace - line_start).c_str()),
							safety_margin)) {
			break;
		}
		space = nextspace;
	}

	if (space > line_start) {
		line_end = space;
		next_line_start = space + 1;
		return;
	}


	// The line didn't fit.
	// We just do a linear search ahead until we hit the max.
	const icu::UnicodeString unicode_word(text.substr(line_start, orig_end).c_str());
	uint32_t line_width = 0;
	int32_t end = -1;
	icu::UnicodeString unicode_line;

	while ((line_width < (m_wrapwidth - safety_margin)) && (end < unicode_word.length())) {
		++end;
		UChar c = unicode_word.charAt(end);
		// Diacritics do not add to the line width
		if (!i18n::is_diacritic(c)) {
			// This only estimates the width
			line_width += m_style.calc_width_for_wrapping(c);
		}
		unicode_line += c;
	}

	// Now make sure that it really fits.
	std::string::size_type test_cutoff = line_start + end * 2 / 3;
	while ((end > 0) && (static_cast<uint32_t>(line_start + end) > test_cutoff)) {
		if (text_width(text.substr(line_start, end), m_style.font->size()) > m_wrapwidth - safety_margin) {
			--end;
		} else {
			break;
		}
	}

	// Find last space
	int32_t last_space = unicode_line.lastIndexOf(0x0020); // space character
	if ((last_space > 0) && (static_cast<uint32_t>(last_space) > minimum_chars)) {
		end = last_space;
	}

	// Make sure that diacritics stay with their base letters, and that
	// start/end line rules are being followed.
	while ((end > 0) && (static_cast<uint32_t>(end) > minimum_chars) &&
			 (i18n::cannot_start_line(unicode_line.charAt(end)) ||
			  i18n::cannot_end_line(unicode_line.charAt(end - 1)))) {
		--end;
	}
	assert(end > 0);

	next_line_start = line_end =
			(i18n::icustring2string(unicode_word.tempSubString(0, end)).size() + line_start);
}


// Returns true if the text won't fit into the alotted width.
bool WordWrap::line_fits(const std::string& text, uint32_t safety_margin) const {
	// calc_width_for_wrapping is fast, but it will underestimate the width.
	// So, we test again with text_width to make sure that the line really fits.
	return m_style.calc_width_for_wrapping(i18n::make_ligatures(text.c_str()))
			<= m_wrapwidth - safety_margin &&
			text_width(text, m_style.font->size()) <= m_wrapwidth - safety_margin;
}


/**
 * Compute the width (including border) of the word-wrapped text.
 *
 * \note This is rather inefficient right now.
 */
uint32_t WordWrap::width() const
{
	uint32_t calculated_width = 0;

	for (uint32_t line = 0; line < m_lines.size(); ++line) {
		uint32_t linewidth = text_width(m_lines[line].text, m_style.font->size());
		if (linewidth > calculated_width)
			calculated_width = linewidth;
	}

	return calculated_width + 2 * LINE_MARGIN;
}

/**
 * Compute the total height of the word-wrapped text.
 */
uint32_t WordWrap::height() const
{
	uint16_t fontheight = 0;
	if (!m_lines.empty()) {
		fontheight = text_height(m_lines[0].text, m_style.font->size());
	}

	return fontheight * (m_lines.size()) + 2 * LINE_MARGIN;
}

/**
 * Given an offset @p caret into the original text, compute the @p line that it
 * appears in in the wrapped text, and also the @p pos within that line (as an offset).
 */
void WordWrap::calc_wrapped_pos(uint32_t caret, uint32_t & line, uint32_t & pos) const
{
	assert(m_lines.size());
	assert(m_lines[0].start == 0);

	uint32_t min = 0;
	uint32_t max = m_lines.size() - 1;

	while (max > min) {
		uint32_t mid = min + (max - min + 1) / 2;

		if (caret >= m_lines[mid].start)
			min = mid;
		else
			max = mid - 1;
	}

	assert(caret >= m_lines[min].start);

	line = min;
	pos = caret - m_lines[min].start;
}

/**
 * Return the starting offset of line number @p line in the original text.
 */
uint32_t WordWrap::line_offset(uint32_t line) const
{
	return m_lines[line].start;
}

/**
 * Draw the word-wrapped text onto \p dst, anchored at \p where with the given alignment.
 *
 * \note This also draws the caret, if any.
 */
void WordWrap::draw(RenderTarget & dst, Point where, Align align, uint32_t caret)
{
	if (m_lines.empty()) return;

	uint32_t caretline, caretpos;

	calc_wrapped_pos(caret, caretline, caretpos);

	if ((align & UI::Align::kVertical) != UI::Align::kTop) {
		uint32_t h = height();

		if ((align & UI::Align::kVertical) == UI::Align::kVCenter)
			where.y -= (h + 1) / 2;
		else
			where.y -= h;
	}

	++where.y;

	Align alignment = mirror_alignment(align);

	uint16_t fontheight = text_height(m_lines[0].text, m_style.font->size());
	for (uint32_t line = 0; line < m_lines.size(); ++line, where.y += fontheight) {
		if (where.y >= dst.height() || int32_t(where.y + fontheight) <= 0)
			continue;

		Point point(where.x, where.y);

		if (static_cast<int>(alignment & UI::Align::kRight)) {
			point.x += m_wrapwidth - LINE_MARGIN;
		}

		const Image* entry_text_im =
				UI::g_fh1->render(as_editorfont(m_lines[line].text,
														  m_style.font->size() - UI::g_fh1->fontset().size_offset(),
														  m_style.fg));
		UI::correct_for_align(alignment, entry_text_im->width(), fontheight, &point);
		dst.blit(point, entry_text_im);

		if (m_draw_caret && line == caretline) {
			std::string line_to_caret = m_lines[line].text.substr(0, caretpos);
			// TODO(GunChleoc): Arabic: Fix cursor position for BIDI text.
			int caret_x = text_width(line_to_caret, m_style.font->size());

			const Image* caret_image = g_gr->images().get("images/ui_basic/caret.png");
			Point caretpt;
			caretpt.x = point.x + caret_x - caret_image->width() + LINE_MARGIN;
			caretpt.y = point.y + (fontheight - caret_image->height()) / 2;
			dst.blit(caretpt, caret_image);
		}
	}
}


} // namespace UI
