/*
 * Copyright (C) 2002, 2006 by the Widelands Development Team
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

#ifndef __S__DESCR_MAINTAINER_H
#define __S__DESCR_MAINTAINER_H

#include <limits>
#include <stdexcept>

/**
 * This template is used to have a typesafe maintaining class for Bob_Descr,
 * Worker_Descr and so on.
 */
template <class T> struct Descr_Maintainer {
	Descr_Maintainer() : capacity(0), nitems(0), items(0) {}
	~Descr_Maintainer();

	static typename T::Index invalid_index()
	{return std::numeric_limits<typename T::Index>::max();}

      T* exists(const char* name);
      int add(T* item);
	typename T::Index get_nitems() const throw () {return nitems;}
      int get_index(const char * const name) const; // can return -1

      T * get(const int idx) const {
         if (idx >= 0 and idx < static_cast<int>(nitems)) return items[idx];
         else return 0;
      }

   private:
	typename T::Index capacity;
	typename T::Index nitems;
	T * * items;

	void reserve(const typename T::Index n) {
		T * * const new_items =
			static_cast<T * * const>(realloc(items, sizeof(T *) * n));
		if (not new_items) throw std::bad_alloc();
		items = new_items;
		capacity = n;
	}
};

template <class T>
int Descr_Maintainer<T>::get_index(const char * const name) const {
   for(typename T::Index i = 0; i < nitems; ++i) {
		if (not strcasecmp(name, items[i]->get_name())) return i;
   }

   return -1;
}

template <class T>
int Descr_Maintainer<T>::add(T* item) {
   nitems++;
	if (nitems >= capacity) {
      reserve(nitems);
   }
   items[nitems-1]=item;
	return nitems-1;
}

//
//returns elemt if it exists, 0 if it doesnt
//
template <class T>
T* Descr_Maintainer<T>::exists(const char* name) {
	for (typename T::Index i = 0; i < nitems; ++i)
      if(!strcasecmp(name, items[i]->get_name())) return items[i];
	return 0;
}

template<class T> Descr_Maintainer<T>::~Descr_Maintainer() {
	for (typename T::Index i = 0; i < nitems; ++i) delete items[i];
	free(items);
}

#endif
