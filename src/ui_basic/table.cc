/*
 * Copyright (C) 2002, 2006-2010 by the Widelands Development Team
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

#include "table.h"

#include "font_handler.h"
#include "graphic/rendertarget.h"
#include "button.h"
#include "mouse_constants.h"
#include "scrollbar.h"
#include "wlapplication.h"

#include "container_iterate.h"

namespace UI {

#define CHECKBOX_VALUE_PREFIX "__[Table<void *>::Table]::checkbox value::"
static std::string const checkbox_value_false = CHECKBOX_VALUE_PREFIX "0";

/**
 * Args: parent  parent panel
 *       x       coordinates of the Table
 *       y
 *       w       dimensions, in pixels, of the Table
 *       h
*/
Table<void *>::Table
	(Panel * const parent,
	 int32_t x, int32_t y, uint32_t w, uint32_t h,
	 const bool descending)
:
	Panel             (parent, x, y, w, h),
	m_max_pic_width   (0),
	m_fontname        (UI_FONT_NAME),
	m_fontsize        (UI_FONT_SIZE_SMALL),
	m_headerheight    (15),
	m_lineheight      (g_fh->get_fontheight(m_fontname, m_fontsize)),
	m_scrollbar       (0),
	m_scrollpos       (0),
	m_selection       (no_selection_index()),
	m_last_click_time (-10000),
	m_last_selection  (no_selection_index()),
	m_sort_column     (0),
	m_sort_descending (descending)
{
	set_think(false);
}


/**
 * Free allocated resources
*/
Table<void *>::~Table()
{
	container_iterate_const(Entry_Record_vector, m_entry_records, i)
		delete *i.current;
}

/// Add a new column to this table.
void Table<void *>::add_column
	(uint32_t            const width,
	 std::string const &       title,
	 Align               const alignment,
	 bool                const is_checkbox_column)
{

	//  If there would be existing entries, they would not get the new column.
	assert(size() == 0);

	uint32_t complete_width = 0;
	container_iterate_const(Columns, m_columns, i)
		complete_width += i.current->width;

	{
		Column const c = {
			title.size() ?
				new Callback_IDButton<Table, Columns::size_type>
					(this,
					 complete_width, 0, width, m_headerheight,
					 g_gr->get_picture(PicMod_UI, "pics/but3.png"),
					 &Table::header_button_clicked, *this, m_columns.size(),
					 title, "", true, false, m_fontname, m_fontsize)
				:
				0,
				width,
				alignment,
				is_checkbox_column
		};
		m_columns.push_back(c);
	}
	if (not m_scrollbar) {
		m_scrollbar =
			new Scrollbar
				(get_parent(),
				 get_x() + get_w() - 24, get_y() + m_headerheight,
				 24,                     get_h() - m_headerheight,
				 false);
		m_scrollbar->moved.set(this, &Table::set_scrollpos);
		m_scrollbar->set_steps(1);
		uint32_t const lineheight = g_fh->get_fontheight(m_fontname, m_fontsize);
		m_scrollbar->set_singlestepsize(lineheight);
		m_scrollbar->set_pagesize(get_h() - lineheight);
	}
}

void Table<void *>::set_column_title
	(uint8_t const col, std::string const & title)
{
	assert(col < m_columns.size());
	Column & column = m_columns.at(col);
	if (not column.btn and title.size()) { //  no title before, but now
		uint32_t complete_width = 0;
		for (uint8_t i = 0; i < col; ++i)
			complete_width += m_columns.at(i).width;
		column.btn =
			new Callback_IDButton<Table, Columns::size_type>
				(this,
				 complete_width, 0, column.width, m_headerheight,
				 g_gr->get_picture(PicMod_UI, "pics/but3.png"),
				 &Table::header_button_clicked, *this, col,
				 title, "", true, false, m_fontname, m_fontsize);
	} else if (column.btn and title.empty()) { //  had title before, not now
		delete column.btn;
		column.btn = 0;
	} else
		column.btn->set_title(title);
}

void Table<void *>::Entry_Record::set_checked
	(uint8_t const col, bool const checked)
{
	_data & cell = m_data.at(col);

	//  The string representation of a checkbox value must be
	//  CHECKBOX_VALUE_PREFIX followed by '0' or '1'.
	assert
		(not cell.d_string.compare
		 	(0, strlen(CHECKBOX_VALUE_PREFIX), CHECKBOX_VALUE_PREFIX));
	assert(cell.d_string.size() == strlen(CHECKBOX_VALUE_PREFIX) + 1);
	assert
		(cell.d_string.at(strlen(CHECKBOX_VALUE_PREFIX)) == '0' or
		 cell.d_string.at(strlen(CHECKBOX_VALUE_PREFIX)) == '1');
	cell.d_string.at(strlen(CHECKBOX_VALUE_PREFIX)) = checked ? '1' : '0';
	cell.d_picture =
		g_gr->get_picture
			(PicMod_UI,
			 checked ? "pics/checkbox_checked.png" : "pics/checkbox_empty.png");
}

void Table<void *>::Entry_Record::toggle     (uint8_t const col) {
	_data & cell = m_data.at(col);

	//  The string representation of a checkbox value must be
	//  CHECKBOX_VALUE_PREFIX followed by '0' or '1'.
	assert
		(not cell.d_string.compare
		 	(0, strlen(CHECKBOX_VALUE_PREFIX), CHECKBOX_VALUE_PREFIX));
	assert(cell.d_string.size() == strlen(CHECKBOX_VALUE_PREFIX) + 1);
	if        (cell.d_string.at(strlen(CHECKBOX_VALUE_PREFIX)) == '0') {
		cell.d_string.at(strlen(CHECKBOX_VALUE_PREFIX)) = '1';
		cell.d_picture =
			g_gr->get_picture(PicMod_UI, "pics/checkbox_checked.png");
	} else if (cell.d_string.at(strlen(CHECKBOX_VALUE_PREFIX)) == '1') {
		cell.d_string.at(strlen(CHECKBOX_VALUE_PREFIX)) = '0';
		cell.d_picture =
			g_gr->get_picture(PicMod_UI, "pics/checkbox_empty.png");
	} else
		assert(false);
}


bool Table<void *>::Entry_Record::is_checked(uint8_t const col) const {
	_data const & cell = m_data.at(col);

	//  The string representation of a checkbox value must be
	//  CHECKBOX_VALUE_PREFIX followed by '0' or '1'.
	assert
		(not cell.d_string.compare
		 	(0, strlen(CHECKBOX_VALUE_PREFIX), CHECKBOX_VALUE_PREFIX));
	assert(cell.d_string.size() == strlen(CHECKBOX_VALUE_PREFIX) + 1);
	assert
		(cell.d_string.at(strlen(CHECKBOX_VALUE_PREFIX)) == '0' or
		 cell.d_string.at(strlen(CHECKBOX_VALUE_PREFIX)) == '1');
	return cell.d_string.at(strlen(CHECKBOX_VALUE_PREFIX)) == '1';
}

Table<void *>::Entry_Record * Table<void *>::find
	(const void * const entry) const
	throw ()
{
	container_iterate_const(Entry_Record_vector, m_entry_records, i)
		if ((*i.current)->entry() == entry)
			return *i.current;

	return 0;
}

/**
 * A header button has been clicked
 */
void Table<void *>::header_button_clicked(Columns::size_type const n) {
	assert(m_columns.at(n).btn);
	if (get_sort_colum() == n) {
		set_sort_descending(not get_sort_descending()); //  change sort direction
		sort();
		return;
	}

	set_sort_column(n);
	sort();
	return;
}

/**
 * Remove all entries from the table
*/
void Table<void *>::clear()
{
	container_iterate_const(Entry_Record_vector, m_entry_records, i)
		delete *i.current;
	m_entry_records.clear();

	if (m_scrollbar)
		m_scrollbar->set_steps(1);
	m_scrollpos = 0;
	m_selection = no_selection_index();
	m_last_click_time = -10000;
	m_last_selection = no_selection_index();
}

/**
 * Redraw the table
*/
void Table<void *>::draw(RenderTarget & dst)
{
	//  draw text lines
	int32_t lineheight = get_lineheight();
	uint32_t idx = m_scrollpos / lineheight;
	int32_t y = 1 + idx * lineheight - m_scrollpos + m_headerheight;

	dst.brighten_rect(Rect(Point(0, 0), get_w(), get_h()), ms_darken_value);

	while (idx < m_entry_records.size()) {
		if (y >= static_cast<int32_t>(get_h()))
			return;

		const Entry_Record & er = *m_entry_records[idx];

		if (idx == m_selection) {
			assert(2 <= get_eff_w());
			dst.brighten_rect
				(Rect(Point(1, y), get_eff_w() - 2, m_lineheight),
				 -ms_darken_value);
		}

		const RGBColor col = er.use_clr ? er.clr : UI_FONT_CLR_FG;

		Columns::size_type const nr_columns = m_columns.size();
		for (uint32_t i = 0, curx = 0; i < nr_columns; ++i) {
			Column const & column    = m_columns[i];
			uint32_t const curw      = column.width;
			Align    const alignment = column.alignment;

			PictureID           const entry_picture = er.get_picture(i);
			std::string const &       entry_string  = er.get_string (i);
			uint32_t w = 0, h = g_fh->get_fontheight(m_fontname, m_fontsize);
			if (entry_picture != g_gr->get_no_picture())
				g_gr->get_picture_size(entry_picture, w, h);
			Point point =
				Point(curx, y)
				+
				Point
					(alignment & Align_Right   ?  curw - w  - 1 :
					 alignment & Align_HCenter ? (curw - w) / 2 :
					 1,
					 (static_cast<int32_t>(get_lineheight())
					  -
					  static_cast<int32_t>(h))
					 /
					 2);
			if (entry_picture != g_gr->get_no_picture())
				dst.blit(point, entry_picture);
			else
				UI::g_fh->draw_string
					(dst,
					 m_fontname, m_fontsize,
					 col,
					 RGBColor(107, 87, 55),
					 point,
					 entry_string, alignment);

			curx += curw;
		}

		y += lineheight;
		++idx;
	}
}

/**
 * Handle mouse presses: select the appropriate entry
 */
bool Table<void *>::handle_mousepress
	(Uint8 const btn, int32_t x, int32_t const y)
{
	switch (btn) {
	case SDL_BUTTON_WHEELDOWN:
	case SDL_BUTTON_WHEELUP:
		return m_scrollbar ? m_scrollbar->handle_mousepress(btn, 0, y) : false;
	case SDL_BUTTON_LEFT: {
		int32_t const time = WLApplication::get()->get_time();

		//  This hick hack is needed if any of the callback functions calls clear
		//  to forget the last clicked time.
		int32_t const real_last_click_time = m_last_click_time;

		m_last_selection  = m_selection;
		m_last_click_time = time;

		uint32_t const row =
			(y + m_scrollpos - m_headerheight) / get_lineheight();
		if (row < m_entry_records.size()) {
			select(row);
			Columns::size_type const nr_cols = m_columns.size();
			for (uint8_t col = 0; col < nr_cols; ++col) {
				Column const & column = m_columns.at(col);
				x -= column.width;
				if (x <= 0) {
					if (column.is_checkbox_column) {
						play_click();
						m_entry_records.at(row)->toggle(col);
					}
					break;
				}
			}
		}

		if //  check if doubleclicked
			(time - real_last_click_time < DOUBLE_CLICK_INTERVAL
			 and
			 m_last_selection == m_selection
			 and m_selection != no_selection_index())
			double_clicked.call(m_selection);

		return true;
	}
	default:
		return false;
	}
}
bool Table<void *>::handle_mouserelease(const Uint8 btn, int32_t, int32_t)
{
	return btn == SDL_BUTTON_LEFT;
}

/**
 * Change the currently selected entry
 *
 * Args: i  the entry to select
 */
void Table<void *>::select(const uint32_t i)
{
	if (m_selection == i)
		return;

	m_selection = i;

	selected.call(m_selection);
	update(0, 0, get_eff_w(), get_h());
}

/**
 * Add a new entry to the table.
*/
Table<void *>::Entry_Record & Table<void *>::add
	(void * const entry, const bool do_select)
{
	int32_t entry_height = g_fh->get_fontheight(m_fontname, m_fontsize);
	if (entry_height > m_lineheight)
		m_lineheight = entry_height;

	Entry_Record & result = *new Entry_Record(entry);
	m_entry_records.push_back(&result);
	result.m_data.resize(m_columns.size());
    for(wl_index_range<Columns::size_type> i(0,m_columns.size());
		 i;++i)
		if (m_columns.at(i.current).is_checkbox_column) {
			result.m_data.at(i.current).d_picture =
				g_gr->get_picture(PicMod_UI, "pics/checkbox_empty.png");
			result.m_data.at(i.current).d_string = checkbox_value_false;
		}

	m_scrollbar->set_steps
		(m_entry_records.size() * get_lineheight()
		 -
		 (get_h() - m_headerheight - 2));

	if (do_select) {
		select(m_entry_records.size() - 1);
		m_scrollbar->set_scrollpos(std::numeric_limits<int32_t>::max());
	}

	update(0, 0, get_eff_w(), get_h());
	return result;
}

/**
 * Scroll to the given position, in pixels.
*/
void Table<void *>::set_scrollpos(int32_t const i)
{
	m_scrollpos = i;

	update(0, 0, get_eff_w(), get_h());
}


void Table<void *>::remove(const uint32_t i) {
	assert(i < m_entry_records.size());

	const Entry_Record_vector::iterator it = m_entry_records.begin() + i;
	delete *it;
	m_entry_records.erase(it);
	if (m_selection == i)
		m_selection = no_selection_index();

	m_scrollbar->set_steps
		(m_entry_records.size() * get_lineheight()
		 -
		 (get_h() - m_headerheight - 2));
}

/**
 * Sort the table alphabetically. make sure that the current selection stays
 * valid (though it might scroll out of visibility).
 * start and end defines the beginning and the end of a subarea to
 * sort, for example you might want to sort directories for themselves at the
 * top of list and files at the bottom.
 */
void Table<void *>::sort(const uint32_t Begin, uint32_t End) {
	assert(m_columns.at(m_sort_column).btn);
	assert(m_sort_column < m_columns.size());
	if (End > size())
		End = size();
	if (get_sort_descending())
		for (uint32_t i = Begin; i != End; ++i)
			for (uint32_t j = i; j != End; ++j) {
				Entry_Record * const eri = m_entry_records[i];
				Entry_Record * const erj = m_entry_records[j];
				if
					(eri->get_string(m_sort_column) > erj->get_string(m_sort_column))
				{
					if      (m_selection == i)
						m_selection = j;
					else if (m_selection == j)
						m_selection = i;
					m_entry_records[i] = erj;
					m_entry_records[j] = eri;
				}
			}
	else
		for (uint32_t i = Begin; i != End; ++i)
			for (uint32_t j = i; j != End; ++j) {
				Entry_Record * const eri = m_entry_records[i];
				Entry_Record * const erj = m_entry_records[j];
				if
					(eri->get_string(m_sort_column) < erj->get_string(m_sort_column))
				{
					if      (m_selection == i)
						m_selection = j;
					else if (m_selection == j)
						m_selection = i;
					m_entry_records[i] = erj;
					m_entry_records[j] = eri;
				}
			}
}


Table<void *>::Entry_Record::Entry_Record(void * const e)
	: m_entry(e), use_clr(false)
{}

void Table<void *>::Entry_Record::set_picture
	(uint8_t const col, PictureID const picid, std::string const & str)
{
	assert(col < m_data.size());

	m_data.at(col).d_picture = picid;
	m_data.at(col).d_string  = str;
}
void Table<void *>::Entry_Record::set_string
	(uint8_t const col, std::string const & str)
{
	assert(col < m_data.size());
	assert
		(str.compare
		 	(0, strlen(CHECKBOX_VALUE_PREFIX), CHECKBOX_VALUE_PREFIX));
	assert
		(m_data.at(col).d_string.compare
		 	(0, strlen(CHECKBOX_VALUE_PREFIX), CHECKBOX_VALUE_PREFIX));

	m_data.at(col).d_picture = g_gr->get_no_picture();
	m_data.at(col).d_string  = str;
}
PictureID Table<void *>::Entry_Record::get_picture(uint8_t const col) const
{
	assert(col < m_data.size());

	return m_data.at(col).d_picture;
}
const std::string & Table<void *>::Entry_Record::get_string
	(uint8_t const col) const
{
	assert(col < m_data.size());

	return m_data.at(col).d_string;
}

}
