/*
 * Copyright (C) 2002, 2006, 2008-2009 by the Widelands Development Team
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


#ifndef UI_TABLE_H
#define UI_TABLE_H

#include "align.h"
#include "panel.h"
#include "m_signal.h"

#include "compile_assert.h"

#include <limits>
#include <vector>

namespace UI {
struct Scrollbar;
template <typename T, typename ID> struct Callback_IDButton;

/// A table with columns and lines. The entries can be sorted by columns by
/// clicking on the column header button.
///
/// Entry can be
///   1. a reference type,
///   2. a pointer type or
///   3. uintptr_t.
template<typename Entry> struct Table {

	struct Entry_Record {
	};

	Table
		(Panel * parent,
		 int32_t x, int32_t y, uint32_t w, uint32_t h,
		 bool descending = false);
	~Table();

	Signal1<uint32_t> selected;
	Signal1<uint32_t> double_clicked;

	/// A column that has a title is sortable (by clicking on the title).
	void add_column
		(uint32_t width,
		 std::string const & title = std::string(),
		 Align                                  = Align_Left,
		 bool                is_checkbox_column = false);

	void set_column_title(uint8_t col, std::string const & title);

	void clear();
	void set_sort_column(uint8_t col) throw ();
	uint8_t get_sort_colum() const throw ();
	bool get_sort_descending() const throw ();

	void sort
		(uint32_t Begin = 0,
		 uint32_t End   = std::numeric_limits<uint32_t>::max());
	void remove(uint32_t);

	Entry_Record & add(void * const entry, const bool select_this = false);

	uint32_t size() const throw ();
	Entry operator[](uint32_t) const throw ();
	static uint32_t no_selection_index() throw ();
	bool has_selection() const throw ();
	uint32_t selection_index() const throw ();
	Entry_Record & get_record(uint32_t) const throw ();
	static Entry get(const Entry_Record &);
	Entry_Record * find(Entry) const throw ();

	void select(uint32_t);
	struct No_Selection : public std::exception {
		char const * what() const throw () {
			return "UI::Table<Entry>: No selection";
		}
	};
	Entry_Record & get_selected_record() const;
	Entry get_selected() const;

	///  Return the total height (text + spacing) occupied by a single line.
	uint32_t get_lineheight() const throw ();

	uint32_t get_eff_w     () const throw ();

	// Drawing and event handling
	void draw(RenderTarget &);
	bool handle_mousepress  (Uint8 btn, int32_t x, int32_t y);
	bool handle_mouserelease(Uint8 btn, int32_t x, int32_t y);
};

template <> struct Table<void *> : public Panel {

	struct Entry_Record {
		Entry_Record(void * entry);

		void set_picture
			(uint8_t col, PictureID picid, std::string const & = std::string());
		void set_string(uint8_t col, std::string const &);
		PictureID get_picture(uint8_t col) const;
		std::string const & get_string(uint8_t col) const;
		void * entry() const throw () {return m_entry;}
		void set_color(const  RGBColor c) {
			use_clr = true;
			clr = c;
		}

		bool     use_color() const throw () {return use_clr;}
		RGBColor get_color() const throw () {return clr;}

		void set_checked(uint8_t col, bool checked);
		void toggle     (uint8_t col);
		bool  is_checked(uint8_t col) const;

	private:
		friend struct Table<void *>;
		void *   m_entry;
		bool     use_clr;
		RGBColor clr;
		struct _data {
			PictureID   d_picture;
			std::string d_string;
		};
		std::vector<_data> m_data;
	};

	Table
		(Panel * parent,
		 int32_t x, int32_t y, uint32_t w, uint32_t h,
		 bool descending = false);
	~Table();

	Signal1<uint32_t> selected;
	Signal1<uint32_t> double_clicked;

	void add_column
		(uint32_t width,
		 std::string const & title = std::string(),
		 Align                                  = Align_Left,
		 bool                is_checkbox_column = false);

	void set_column_title(uint8_t col, std::string const & title);

	void clear();
	void set_sort_column(uint8_t const col) {
		assert(col < m_columns.size());
		m_sort_column = col;
	}
	uint8_t get_sort_colum() const throw () {return m_sort_column;}
	bool  get_sort_descending() const throw () {return m_sort_descending;}
	void set_sort_descending(bool const descending) {
		m_sort_descending = descending;
	}
	void set_font(std::string const & fontname, int32_t const fontsize) {
		m_fontname = fontname;
		m_fontsize = fontsize;
		m_headerheight = fontsize * 6 / 5;
	}

	void sort
		(uint32_t Begin = 0,
		 uint32_t End   = std::numeric_limits<uint32_t>::max());
	void remove(uint32_t);

	Entry_Record & add(void * entry = 0, bool select = false);

	uint32_t size() const throw () {return m_entry_records.size();}
	void * operator[](uint32_t const i) const {
		assert(i < m_entry_records.size());
		return m_entry_records[i]->entry();
	}
	static uint32_t no_selection_index() {
		return std::numeric_limits<uint32_t>::max();
	}
	bool has_selection() const {
		return m_selection != no_selection_index();
	}
	uint32_t selection_index() const throw () {return m_selection;}
	Entry_Record & get_record(uint32_t const n) const {
		assert(n < m_entry_records.size());
		return *m_entry_records[n];
	}
	static void * get(Entry_Record const & er) {return er.entry();}
	Entry_Record * find(const void * entry) const throw ();

	void select(uint32_t);
	struct No_Selection : public std::exception {
		char const * what() const throw () {
			return "UI::Table<void *>: No selection";
		}
	};
	Entry_Record & get_selected_record() const {
		if (m_selection == no_selection_index())
			throw No_Selection();
		assert(m_selection < m_entry_records.size());
		return *m_entry_records.at(m_selection);
	}
	void remove_selected() throw (No_Selection) {
		if (m_selection == no_selection_index())
			throw No_Selection();
		remove(m_selection);
	}
	void * get_selected() const {return get_selected_record().entry();};

	uint32_t get_lineheight() const throw () {return m_lineheight + 2;}
	uint32_t get_eff_w     () const throw () {return get_w();}

	// Drawing and event handling
	void draw(RenderTarget &);
	bool handle_mousepress  (Uint8 btn, int32_t x, int32_t y);
	bool handle_mouserelease(Uint8 btn, int32_t x, int32_t y);


private:
	struct Column;
	typedef std::vector<Column> Columns;
	struct Column {
		Callback_IDButton<Table, Columns::size_type> * btn;
		uint32_t                              width;
		Align                                 alignment;
		bool                                           is_checkbox_column;
	};

	static const int32_t ms_darken_value = -20;

	Columns            m_columns;
	uint32_t           m_max_pic_width;
	std::string        m_fontname;
	uint32_t           m_fontsize;
	uint32_t           m_headerheight;
	int32_t            m_lineheight;
	Scrollbar        * m_scrollbar;
	int32_t            m_scrollpos; //  in pixels
	uint32_t           m_selection;
	int32_t            m_last_click_time;
	uint32_t           m_last_selection;  // for double clicks
	Columns::size_type m_sort_column;
	bool               m_sort_descending;
	
	bool               m_needredraw;
	PictureID          m_cache_pid;
	
	void header_button_clicked(Columns::size_type);
	typedef std::vector<Entry_Record *> Entry_Record_vector;
	Entry_Record_vector m_entry_records;
	void set_scrollpos(int32_t pos);
};

template <typename Entry>
	struct Table<const Entry * const> : public Table<void *>
{
	typedef Table<void *> Base;
	Table
		(Panel * parent,
		 int32_t x, int32_t y, uint32_t w, uint32_t h,
		 const bool descending = false)
		: Base(parent, x, y, w, h, descending)
	{}

	Entry_Record & add
		(Entry const * const entry = 0, bool const select_this = false)
	{
		return Base::add(const_cast<Entry *>(entry), select_this);
	}

	Entry const * operator[](uint32_t const i) const throw () {
		return static_cast<Entry const *>(Base::operator[](i));
	}

	static Entry const * get(const Entry_Record & er) {
		return static_cast<Entry const *>(er.entry());
	}

	Entry const * get_selected() const {
		return static_cast<Entry const *>(Base::get_selected());
	}
};

template <typename Entry> struct Table<Entry * const> : public Table<void *> {
	typedef Table<void *> Base;
	Table
		(Panel * parent,
		 int32_t x, int32_t y, uint32_t w, uint32_t h,
		 const bool descending = false)
		: Base(parent, x, y, w, h, descending)
	{}

	Entry_Record & add(Entry * const entry = 0, bool const select_this = false)
	{
		return Base::add(entry, select_this);
	}

	Entry * operator[](uint32_t const i) const {
		return static_cast<Entry *>(Base::operator[](i));
	}

	static Entry * get(Entry_Record const & er) {
		return static_cast<Entry *>(er.entry());
	}

	Entry * get_selected() const {
		return static_cast<Entry *>(Base::get_selected());
	}
};

template <typename Entry> struct Table<const Entry &> : public Table<void *> {
	typedef Table<void *> Base;
	Table
		(Panel * parent,
		 int32_t x, int32_t y, uint32_t w, uint32_t h,
		 const bool descending = false)
		: Base(parent, x, y, w, h, descending)
	{}

	Entry_Record & add(Entry const & entry, bool const select_this = false) {
		return Base::add(&const_cast<Entry &>(entry), select_this);
	}

	Entry const & operator[](uint32_t const i) const {
		return *static_cast<Entry const *>(Base::operator[](i));
	}

	static Entry const & get(Entry_Record const & er) {
		return *static_cast<Entry const *>(er.entry());
	}

	Entry_Record * find(Entry const & entry) const {
		return Base::find(&entry);
	}

	Entry const & get_selected() const {
		return *static_cast<Entry const *>(Base::get_selected());
	}
};

template <typename Entry> struct Table<Entry &> : public Table<void *> {
	typedef Table<void *> Base;
	Table
		(Panel * parent,
		 int32_t x, int32_t y, uint32_t w, uint32_t h,
		 const bool descending = false)
		: Base(parent, x, y, w, h, descending)
	{}

	Entry_Record & add(Entry & entry, bool const select_this = false) {
		return Base::add(&entry, select_this);
	}

	Entry & operator[](uint32_t const i) const {
		return *static_cast<Entry *>(Base::operator[](i));
	}

	static Entry & get(Entry_Record const & er) {
		return *static_cast<Entry *>(er.entry());
	}

	Entry_Record * find(Entry & entry) const {return Base::find(&entry);}

	Entry & get_selected() const {
		return *static_cast<Entry *>(Base::get_selected());
	}
};

compile_assert(sizeof(void *) == sizeof(uintptr_t));
template <> struct Table<uintptr_t> : public Table<void *> {
	typedef Table<void *> Base;
	Table
		(Panel * parent,
		 int32_t x, int32_t y, uint32_t w, uint32_t h,
		 const bool descending = false)
		: Base(parent, x, y, w, h, descending)
	{}

	Entry_Record & add(uintptr_t const entry, bool const select_this = false) {
		return Base::add(reinterpret_cast<void *>(entry), select_this);
	}

	uintptr_t operator[](uint32_t const i) const throw () {
		return reinterpret_cast<uintptr_t>(Base::operator[](i));
	}
	static uintptr_t get(Entry_Record const & er) {
		return reinterpret_cast<uintptr_t>(er.entry());
	}

	Entry_Record * find(uintptr_t const entry) const {
		return Base::find(reinterpret_cast<void const *>(entry));
	}

	uintptr_t get_selected() const {
		return reinterpret_cast<uintptr_t>(Base::get_selected());
	}
};
template <> struct Table<uintptr_t const> : public Table<uintptr_t> {
	typedef Table<uintptr_t> Base;
	Table
		(Panel * parent,
		 int32_t x, int32_t y, uint32_t w, uint32_t h,
		 const bool descending = false)
		: Base(parent, x, y, w, h, descending)
	{}
};

}

#endif
