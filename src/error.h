/*
 * Copyright (C) 2002, 2003 by the Widelands Development Team
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

#ifndef ERROR_H
#define ERROR_H

#include "types.h"

#ifdef __GNUC__
#define PRINTF_FORMAT(b,c) __attribute__ (( __format__ (__printf__,b,c) ))
#else
#define PRINTF_FORMAT(b,c)
#endif

/*
==============================================================================

ERROR HANDLING FUNCTIONS

==============================================================================
*/

#define RET_OK 				0
#define RET_EXIT           1
#define ERR_NOT_IMPLEMENTED -1
#define ERR_INVAL_ARG -2
#define ERR_FAILED -255


// Critical errors, displayed to the user.
// Does not return (unless the user is really daring)
void critical_error(const char *, ...) PRINTF_FORMAT(1,2);

// Informational messages that can aid in debugging
#define ALIVE() log("Alive in %s line %i\n", __FILE__, __LINE__)
void log(const char *, ...) PRINTF_FORMAT(1,2);

#ifdef DEBUG
   #ifndef KEEP_STANDART_ASSERT
      #ifdef assert
         #undef assert
      #endif

/* reintroduce when we figure out a way to actually manage the beast that is autotools
   (problem is: tools include this as well)
      extern int graph_is_init;

      inline void myassert(int line, const char* file, int cond, const char* condt) {
         if(!cond) {
            char buf[200];
            sprintf(buf, "%s (%i): assertion \"%s\" failed!\n", file, line, condt);

            if(graph_is_init) {
               critical_error(buf);
               // User chooses, if it goes on
            } else {
               tell_user(buf);
               exit(-1);
            }
         }
      }
*/
		void myassert(int line, const char* file, const char* condt) throw(wexception);
   
      #define assert(condition) \
			do { if (!(condition)) myassert(__LINE__, __FILE__, #condition); } while(0)

	#endif
#else
      #define NDEBUG 1
      #include <assert.h>
#endif

#endif
