/*
 * Copyright (C) 2001 by Holger Rapp 
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

#include "font.h"

/** class Font_Handler
 *
 * This class generates font Pictures out of strings and returns them
 * This is sure that the user want's fixed font strings
 *
 * It's a singleton
 * 
 * DEPENDS: class	Graph::Pic
 */

/** Font_Handler::Font_Handler(void)
 *
 * Simple inits
 *
 * Agrs: None
 * Returns: Nothing
 */
Font_Handler::Font_Handler(void) {
		  for (unsigned int i=0; i<MAX_FONTS; i++) {
					 w[i]=0;
					 h[i]=0;
					 pics[i]=0;
		  }
}

/** Font_Handler::~Font_Handler(void) 
 *
 * Simple cleanups
 *
 * Args: None
 * Returns: Nothing
 */
Font_Handler::~Font_Handler(void) {
		  for (unsigned int i=0; i<MAX_FONTS; i++) {
					 w[i]=0;
					 h[i]=0;
					 if(pics[i]) delete pics[i];
		  }
}

/** void Font_Handler::set_font(unsigned short f, Graph::Pic * p, unsigned short w, unsigned short h)
 *
 * This registers a certain font with the given
 * objects
 *
 * Args:	f 	number of font to register
 * 	  	p	Pic to register
 * 	  	w	width of one char
 * 	  	h	height of one char
 *	Returns: Nothing
 */
void Font_Handler::set_font(unsigned short f, Graph::Pic * p, unsigned short gw, unsigned short gh) {
		  assert(f<MAX_FONTS && "attempt to register a font with a big number, which is not allowed!");
		  assert(p);

		  w[f]=gw;
		  h[f]=gh;
		  pics[f]=p;
}
		 
/** Pic* Font_Handler::get_string(const unsigned char* str, const unsigned short f);
 *
 * This function constructs a Picture containing the given text and
 * returns it. It just makes ONE line. Not a whole paragraph
 * 
 * Args:	str	String to construct
 * 		f		Font to use
 * Returns:	Pointer to picture, caller must free it later on
 */
Pic* Font_Handler::get_string(const char* str, const unsigned short f) {
		  assert(f<MAX_FONTS && "attempt to get a string with a font with a big number, which is not allowed!");
		  assert(pics[f]);
		  
		  char buf[strlen(str)+1];
		  unsigned char c;
		  unsigned int n=0;
		  unsigned int x=0;
		  
		  for(unsigned int i=0; i<strlen(str); i++) {
					 c=(unsigned char) str[i];
					 if(c=='\t' || c=='\r' || c=='\n' || c=='\b' || c=='\a') continue;
					 buf[n]=c;
					 ++n;
					 
		  }
		  buf[n]='\0';
		  
		  // Now buf contains only valid chars
		  Pic* retval=new Pic;
		  retval->set_size(strlen(buf)*w[f], h[f]);
		  retval->set_clrkey(pics[f]->get_clrkey());

		  for(unsigned int i=0; i<strlen(buf); i++) {
					 c=buf[i];
					 if(c < 32  || c > 127) {
								// c is NOT an international ASCII char, we skip it silently
								c=127;
					 }

					 // change c, so we get correct offsets in our font file
					 c-=32;

					 Graph::copy_pic(retval, pics[f], x, 0, c*w[f], 0, w[f], h[f]);
					 x+=w[f];
		  }
 
		  return retval;
}
					 
