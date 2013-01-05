/*
 * Copyright (C) 2013 by the Widelands Development Team
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
#ifndef COMPILE_DIAGNOSTICS_H
#define COMPILE_DIAGNOSTICS_H

/* Macros for disabling GCC warnings and errors
 * From http://dbp-consulting.com/tutorials/SuppressingGCCWarnings.html and
 * slightly modified for GCC 4.2-4.5
 */
#if ((__GNUC__ * 100) + __GNUC_MINOR__) >= 402
# define GCC_DIAG_STR(s) #s
# define GCC_DIAG_JOINSTR(x, y) GCC_DIAG_STR(x ## y)
# define GCC_DIAG_DO_PRAGMA(x) _Pragma (#x)
# define GCC_DIAG_PRAGMA(x) GCC_DIAG_DO_PRAGMA(GCC diagnostic x)
# if ((__GNUC__ * 100) + __GNUC_MINOR__) >= 406
#  define GCC_DIAG_OFF(x) GCC_DIAG_PRAGMA(push) \
          GCC_DIAG_PRAGMA(ignored x)
#  define GCC_DIAG_ON(x) GCC_DIAG_PRAGMA(pop)
# else
#  define GCC_DIAG_OFF(x) GCC_DIAG_DO_PRAGMA(message Ignore x)
#  define GCC_DIAG_ON(x) GCC_DIAG_DO_PRAGMA(message End of ignore)
# endif
#else
# define GCC_DIAG_OFF(x)
# define GCC_DIAG_ON(x)
#endif


#endif
