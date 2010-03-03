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

#include "listselect.h"

#include "constants.h"
#include "font_handler.h"
#include "graphic/rendertarget.h"
#include "wlapplication.h"
#include "log.h"

#include "container_iterate.h"

#include <iostream>

namespace UI {
/**
 * Initialize a list select panel
 *
 * Args: parent  parent panel
 *       x       coordinates of the Listselect
 *       y
 *       w       dimensions, in pixels, of the Listselect
 *       h
 *       align   alignment of text inside the Listselect
*/
BaseListselect::BaseListselect
	(Panel * const parent,
	 int32_t const x, int32_t const y, uint32_t const w, uint32_t const h,
	 Align const align, bool const show_check)
	:
	Panel(parent, x, y, w, h),
	m_lineheight(g_fh->get_fontheight(UI_FONT_SMALL)),
	m_scrollbar      (this, get_w() - 24, 0, 24, h, false),
	m_scrollpos     (0),
	m_selection     (no_selection_index()),
	m_last_click_time(-10000),
	m_last_selection(no_selection_index()),
	m_show_check(show_check),
	m_fontname(UI_FONT_NAME),
	m_fontsize(UI_FONT_SIZE_SMALL),
	m_needredraw(true)
{
	set_think(false);

	set_align(align);

	m_scrollbar.moved.set(this, &BaseListselect::set_scrollpos);
	m_scrollbar.set_singlestepsize(g_fh->get_fontheight(m_fontname, m_fontsize));
	m_scrollbar.set_pagesize
		(h - 2 * g_fh->get_fontheight(m_fontname, m_fontsize));
	m_scrollbar.set_steps(1);

	if (show_check) {
		uint32_t pic_h;
		m_check_picid = g_gr->get_picture(PicMod_Game,  "pics/list_selected.png");
		g_gr->get_picture_size(m_check_picid, m_max_pic_width, pic_h);
		if (pic_h > m_lineheight)
			m_lineheight = pic_h;
	}
	else
		m_max_pic_width = 0;
}


/**
 * Free allocated resources
*/
BaseListselect::~BaseListselect()
{
	clear();
}


/**
 * Remove all entries from the listselect
*/
void BaseListselect::clear() {
	container_iterate_const(Entry_Record_deque, m_entry_records, i)
		delete *i.current;
	m_entry_records.clear();

	m_scrollbar.set_steps(1);
	m_scrollpos = 0;
	m_selection = no_selection_index();
	m_last_click_time = -10000;
	m_last_selection = no_selection_index();
}


/**
 * Add a new entry to the listselect.
 *
 * Args: name   name that will be displayed
 * entry  value returned by get_select()
 *       sel    if true, directly select the new entry
*/
void BaseListselect::add
	(char const * const name,
	 uint32_t           entry,
	 PictureID    const picid,
	 bool         const sel)
{
	Entry_Record * er = new Entry_Record();

	er->m_entry = entry;
	er->picid = picid;
	er->use_clr = false;
	er->name = std::string(name);
	uint32_t entry_height = 0;
	if (picid == g_gr->get_no_picture()) {
		entry_height = g_fh->get_fontheight(m_fontname, m_fontsize);
	} else {
		uint32_t w, h;
		g_gr->get_picture_size(picid, w, h);
		entry_height = (h >= g_fh->get_fontheight(m_fontname, m_fontsize))
			? h : g_fh->get_fontheight(m_fontname, m_fontsize);
		if (m_max_pic_width < w)
			m_max_pic_width = w;
	}

	if (entry_height > m_lineheight)
		m_lineheight = entry_height;

	m_entry_records.push_back(er);

	m_scrollbar.set_steps(m_entry_records.size() * get_lineheight() - get_h());
	
	m_needredraw = true;
	update(0, 0, get_eff_w(), get_h());

	if (sel)
		select(m_entry_records.size() - 1);
}

void BaseListselect::add_front
	(char const * const name,
	 PictureID    const picid,
	 bool         const sel)
{
	Entry_Record * er = new Entry_Record();

	er->m_entry = 0;
	container_iterate_const(Entry_Record_deque, m_entry_records, i)
		++(*i.current)->m_entry;

	er->picid = picid;
	er->use_clr = false;
	//strcpy(er.name, name);
	er->name = std::string(name);

	uint32_t entry_height = 0;
	if (picid == g_gr->get_no_picture())
		entry_height = g_fh->get_fontheight(m_fontname, m_fontsize);
	else {
		uint32_t w, h;
		g_gr->get_picture_size(picid, w, h);
		entry_height = (h >= g_fh->get_fontheight(m_fontname, m_fontsize))
			? h : g_fh->get_fontheight(m_fontname, m_fontsize);
		if (m_max_pic_width < w)
			m_max_pic_width = w;
	}

	if (entry_height > m_lineheight)
		m_lineheight = entry_height;

	m_entry_records.push_front(er);

	m_scrollbar.set_steps(m_entry_records.size() * get_lineheight() - get_h());

	m_needredraw = true;
	update(0, 0, get_eff_w(), get_h());

	if (sel)
		select(0);
}

/**
 * Switch two entries
 */
void BaseListselect::switch_entries(const uint32_t m, const uint32_t n)
{
	assert(m < size());
	assert(n < size());

	std::swap(m_entry_records[m], m_entry_records[n]);

	if (m_selection == m) {
		m_selection = n;
		selected.call(n);
	} else if (m_selection == n) {
		m_selection = m;
		selected.call(m);
	}
}

/**
 * Sort the listbox alphabetically. make sure that the current selection stays
 * valid (though it might scroll out of visibility).
 * start and end defines the beginning and the end of a subarea to
 * sort, for example you might want to sort directories for themselves at the
 * top of list and files at the bottom.
 */
void BaseListselect::sort(const uint32_t Begin, uint32_t End)
{
	if (End > size())
		End = size();
	for (uint32_t i = Begin; i < End; ++i)
		for (uint32_t j = i + 1; j < End; ++j) {
			Entry_Record * const eri = m_entry_records[i];
			Entry_Record * const erj = m_entry_records[j];
			if (strcmp(eri->name.c_str(), erj->name.c_str()) > 0) {
				if      (m_selection == i)
					m_selection = j;
				else if (m_selection == j)
					m_selection = i;
				m_entry_records[i] = erj;
				m_entry_records[j] = eri;
			}
		}
}

/**
 * Set the list alignment (only horizontal alignment works)
*/
void BaseListselect::set_align(const Align align)
{
	m_align = static_cast<Align>(align & Align_Horizontal);
}


/**
 * Scroll to the given position, in pixels.
*/
void BaseListselect::set_scrollpos(const int32_t i)
{
	m_scrollpos = i;

	m_needredraw = true;
	update(0, 0, get_eff_w(), get_h());
}


/**
 * Define a special color that will be used to display the item at the given
 * index.
 */
void BaseListselect::set_entry_color
	(const uint32_t n, const RGBColor col) throw ()
{
	assert(n < m_entry_records.size());

	m_entry_records[n]->use_clr = true;
	m_entry_records[n]->clr = col;
}


/**
 * Change the currently selected entry
 *
 * Args: i  the entry to select
 */
void BaseListselect::select(const uint32_t i)
{
	if (m_selection == i)
		return;

	if (m_show_check) {
		if (m_selection != no_selection_index())
			m_entry_records[m_selection]->picid = g_gr->get_no_picture();
		m_entry_records[i]->picid = m_check_picid;
	}
	m_selection = i;

	m_needredraw = true;
	selected.call(m_selection);
	update(0, 0, get_eff_w(), get_h());
}

/**
 * \return \c true if an item is select, or \c false if there is no current
 * selection
 */
bool BaseListselect::has_selection() const throw ()
{
	return m_selection != no_selection_index();
}


/**
 * \return the ID/entry value of the currently selected item.
 * The entry value is given as a parameter to \ref add
 *
 * Throws an exception when no item is selected.
 */
uint32_t BaseListselect::get_selected() const throw (No_Selection)
{
	if (m_selection == no_selection_index())
		throw No_Selection();

	return m_entry_records[m_selection]->m_entry;
}


/**
 * Remove the currently selected item. Throws an exception when no
 * item is selected.
 */
void BaseListselect::remove_selected() throw (No_Selection)
{
	if (m_selection == no_selection_index())
		throw No_Selection();

	remove(m_selection);
}


uint32_t BaseListselect::get_lineheight() const throw ()
{
	return m_lineheight + 2;
}

uint32_t BaseListselect::get_eff_w() const throw ()
{
	return get_w();
}

/**
Redraw the listselect box
*/
void BaseListselect::draw(RenderTarget & odst)
{
	// Only render if something changed
	if(!m_needredraw)
	{
		odst.blit(Point(0, 0), m_cache_pid);
		return;
	}

	// Create a off-screen surface 
	m_cache_pid = g_gr->create_surface_a(odst.get_w(), odst.get_h());

	m_cache_pid->surface->fill_rect(Rect(Point(0, 0), get_w(), get_h()), RGBAColor(0, 0, 0, 80));

	RenderTarget &dst = *(g_gr->get_surface_renderer(m_cache_pid));

	// draw text lines
	const uint32_t lineheight = get_lineheight();
	uint32_t idx = m_scrollpos / lineheight;
	int32_t y = 1 + idx * lineheight - m_scrollpos;

	//dst.brighten_rect(Rect(Point(0, 0), get_w(), get_h()), ms_darken_value);

	while (idx < m_entry_records.size()) {
		assert
			(get_h()
			 <
			 static_cast<int32_t>(std::numeric_limits<int32_t>::max()));

		if (y >= static_cast<int32_t>(get_h()))
			break;

		const Entry_Record & er = *m_entry_records[idx];

		// Highlight the current selected entry
		if (idx == m_selection) {
			Rect r = Rect(Point(1, y), get_eff_w() - 2, m_lineheight);
			if( r.x < 0 ) {
				r.w+=r.x; r.x=0;
			}
			if( r.y < 0 ) {
				r.h+=r.y; r.y=0;
			}
			assert(2 <= get_eff_w());
			// Make the area a bit more white and more transparent
			if( r.w > 0 and r.h > 0 )
				m_cache_pid->surface->fill_rect(r, RGBAColor(200, 200, 200, 30));
			//dst.brighten_rect
			//(Rect(Point(1, y), get_eff_w() - 2, m_lineheight),
			// -ms_darken_value*2);
		}


		int32_t const x =
			m_align & Align_Right   ? get_eff_w() -      1 :
			m_align & Align_HCenter ? get_eff_w() >>     1 :

			// Pictures are always left aligned, leave some space here
			m_max_pic_width         ? m_max_pic_width + 10 :
			1;

		const RGBColor col = er.use_clr ? er.clr : UI_FONT_CLR_FG;

		// Horizontal center the string
		UI::g_fh->draw_string
			(dst,
			 m_fontname, m_fontsize,
			 col,
			 RGBColor(107, 87, 55),
			 Point
			 	(x,
			 	 y +
			 	 (get_lineheight() - g_fh->get_fontheight(m_fontname, m_fontsize))
			 	 /
			 	 2),
			 er.name,
			 m_align);

		// Now draw pictures

		if (er.picid != g_gr->get_no_picture()) {
			uint32_t w, h;
			g_gr->get_picture_size(er.picid, w, h);
			dst.blit_a(Point(1, y + (get_lineheight() - h) / 2), er.picid, false);
		}

		y += lineheight;
		++idx;
	}
	//SDL_SetAlpha(&surf, SDL_SRCALPHA, 0);
	odst.blit(Point(0, 0), m_cache_pid);
	m_needredraw = false;
}

/**
 * Handle mouse presses: select the appropriate entry
 */
bool BaseListselect::handle_mousepress(const Uint8 btn, int32_t, int32_t y)
{
	switch (btn) {
	case SDL_BUTTON_WHEELDOWN:
	case SDL_BUTTON_WHEELUP:
		return m_scrollbar.handle_mousepress(btn, 0, y);
	case SDL_BUTTON_LEFT: {
		int32_t const time = WLApplication::get()->get_time();

		//  This hick hack is needed if any of the callback functions calls clear
		//  to forget the last clicked time.
		int32_t const real_last_click_time = m_last_click_time;

		m_last_selection  = m_selection;
		m_last_click_time = time;

		y = (y + m_scrollpos) / get_lineheight();
		if (y < 0 or static_cast<int32_t>(m_entry_records.size()) <= y)
			return false;
		play_click();
		select(y);
		clicked.call(m_selection);

		if //  check if doubleclicked
			(time - real_last_click_time < DOUBLE_CLICK_INTERVAL
			 and
			 m_last_selection == m_selection
			 and
			 m_selection != no_selection_index())
			double_clicked.call(m_selection);

		return true;
	}
	default:
		return false;
	}
}

bool BaseListselect::handle_mouserelease(const Uint8 btn, int32_t, int32_t)
{
	return btn == SDL_BUTTON_LEFT;
}

/**
 * Remove entry
 */
void BaseListselect::remove(const uint32_t i)
{
	assert(i < m_entry_records.size());

	delete (m_entry_records[i]);
	m_entry_records.erase(m_entry_records.begin() + i);
	if (m_selection == i)
		selected.call(m_selection = no_selection_index());
	else if (i <  m_selection)
		--m_selection;
}

/**
 * Remove an entry by name. This only removes
 * the first entry with this name. If none is found, nothing
 * is done
 */
void BaseListselect::remove(const char * const str)
{
	for (uint32_t i = 0; i < m_entry_records.size(); ++i) {
		if (!strcmp(m_entry_records[i]->name.c_str(), str)) {
			remove(i);
			return;
		}
	}
}

}
