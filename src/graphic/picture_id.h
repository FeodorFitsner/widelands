/*
 * Copyright (C) 2002-2004, 2006-2009 by the Widelands Development Team
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
 *
 */
#ifndef PICTURE_ID_H
#define PICTURE_ID_H

#include <boost/shared_ptr.hpp>

struct IPicture;

// TODO(sirver): This should not really be a shared pointer. It is clear
// who owns this usually.
// TODO(sirver): Get rid of this as fast as possible. Horrible!
typedef boost::shared_ptr<IPicture> PictureID;

#endif
