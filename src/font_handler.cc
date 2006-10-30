/*
 * Copyright (C) 2002-2006 by the Widelands Development Team
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
 * Some Methods taken from Wesnoth.
 * http://www.wesnoth.org
 */

#include <SDL_image.h>
#include <algorithm>
#include <iostream>
#include <SDL_ttf.h>
#include "error.h"
#include "filesystem.h"
#include "font_handler.h"
#include "font_loader.h"
#include "graphic.h"
#include "rendertarget.h"
#include "util.h"
#include "wexception.h"
#include "text_parser.h"

// GRAPHIC_TODO: remove this include
#include "graphic_impl.h"

/*
 * Plain Constructor
 */
Font_Handler::Font_Handler(void) {
	if(TTF_Init()==-1) throw wexception("True Type library did not initialize: %s\n", TTF_GetError());
	m_font_loader = new Font_Loader();
	m_varcallback = 0;
	m_cbdata = 0;
}

/*
 * Plain Destructor
 */
Font_Handler::~Font_Handler(void) {
	delete m_font_loader;
	TTF_Quit();
}


/*
 * Returns the height of the font, in pixels.
*/
uint Font_Handler::get_fontheight(const std::string & name, const int size) {
	TTF_Font* f = m_font_loader->get_font(name,size);
	const int fontheight = TTF_FontHeight(f);
	if (fontheight < 0) throw wexception
		("TTF_FontHeight returned a negative value, which does not have a known "
		 "meaning.");
	return fontheight;
}

/*
 * Draw this string, if it is not cached, create the cache for it.
 *
 * The whole text block is rendered in one Surface, this surface is cached
 * for reuse.
 * This is a really fast approach for static texts, but for text areas which keep changing
 * (like Multiline editboxes or chat windows, debug windows ... ) this is the death, for a whole new
 * surface is rendered with everything that has been written so far.
 */
// TODO: rename this to draw text
void Font_Handler::draw_string(RenderTarget* dst, std::string font, int size, RGBColor fg, RGBColor bg, int dstx, int dsty,
                               std::string text, Align align, int wrap, Widget_Cache widget_cache, uint *widget_cache_id, int caret) {
	TTF_Font* f = m_font_loader->get_font(font,size);
	//Width and height of text, needed for alignment
	uint w, h;
	uint picid;
	//Fontrender takes care of caching
	if (widget_cache == Widget_Cache_None) {
		// look if text is cached
		_Cache_Infos  ci = {
		                       0,
		                       text,
		                       f,
		                       fg,
		                       bg,
		                       0,
		                       0,
		                   };

		std::list<_Cache_Infos>::iterator i=find(m_cache.begin(), m_cache.end(), ci);

		if (i!=m_cache.end())  {
			// Ok, it is cached, blit it and done
			picid = i->surface_id;
			w = i->w;
			h = i->h;
			if (i!=m_cache.begin()) {
				m_cache.push_front (*i);
				m_cache.erase (i);
			}
		}
		else {
			//not cached, create a new surface
			ci.surface_id = create_text_surface(f, fg, bg, text, align, wrap);
			// Now cache it
			g_gr->get_picture_size(ci.surface_id, ci.w, ci.h);
			ci.f = f;
			m_cache.push_front (ci);

			while( m_cache.size() > CACHE_ARRAY_SIZE) {
				g_gr->free_surface(m_cache.back().surface_id);
				m_cache.pop_back();
			}
			//Set for alignment and blitting
			picid = ci.surface_id;
			w = ci.w;
			h = ci.h;
		}
	}
	//Widget gave us an explicit picid
	else if (widget_cache == Widget_Cache_Use) {
		g_gr->get_picture_size(*widget_cache_id, w, h);
		picid = *widget_cache_id;
	}
	//We need to (re)create the picid for the widget
	else {
		if (widget_cache == Widget_Cache_Update)
			g_gr->free_surface(*widget_cache_id);
		*widget_cache_id = create_text_surface(f, fg, bg, text, align, wrap,caret);
		g_gr->get_picture_size(*widget_cache_id, w, h);
		picid = *widget_cache_id;
	}
	do_align(align,&dstx,&dsty,w,h);
	dst->blit(dstx, dsty, picid);
}

/*
* Creates a Widelands surface of the given text, checks if multiline or not
*/
uint Font_Handler::create_text_surface(TTF_Font* f, RGBColor fg, RGBColor bg,
                                       std::string text, Align align, int wrap, int caret) {
	SDL_Surface *surface = (wrap > 0 ? create_static_long_text_surface(f, fg, bg, text, align, wrap, 0, caret)
	                        : create_single_line_text_surface(f, fg, bg, text, align, caret));
	return convert_sdl_surface(surface);
}

/*
 * This function renders a short (single line) text surface
 */
SDL_Surface* Font_Handler::create_single_line_text_surface
(TTF_Font* f, RGBColor fg, RGBColor bg, std::string text, Align, int caret)
{
	// render this block in a SDL Surface
	SDL_Color sdl_fg = { fg.r(), fg.g(), fg.b(),0 };
	SDL_Color sdl_bg = { bg.r(), bg.g(), bg.b(),0 };

	SDL_Surface *surface;

	if( !text.size() )
		text = " ";

	if (!(surface = TTF_RenderUTF8_Shaded(f, text.c_str(), sdl_fg, sdl_bg))) {
		log("Font_Handler::create_single_line_text_surface, an error : %s\n", TTF_GetError());
		log("Text was: '%s'\n", text.c_str());
		return 0; // This will skip this line hopefully
	}

	if (caret != -1) {
		std::string text_caret_pos = text.substr(0,caret);
		render_caret(f,surface,text_caret_pos);
	}

	return surface;
}

/*
 * This function renders a longer (multiline) text passage, which should not change.
 * If it changes, this function is highly unperformant.
 *
 * This function also completly ignores vertical alignement
 * Horizontal alignment is now recognized correctly
 */
SDL_Surface* Font_Handler::create_static_long_text_surface(TTF_Font* f, RGBColor fg, RGBColor bg,
        std::string text, Align align, int wrap, int line_spacing, int caret) {
	assert( wrap > 0);
	assert( text.size() > 0 );

	int global_surface_width  = wrap > 0 ? wrap : 0;
	int global_surface_height = 0;
	std::vector<SDL_Surface*> m_rendered_lines;
	std::vector<std::string> lines;

	text = word_wrap_text(f,text,wrap);
	split_string(text, &lines, "\n");

	SDL_Color sdl_fg = { fg.r(), fg.g(), fg.b(),0 };
	SDL_Color sdl_bg = { bg.r(), bg.g(), bg.b(),0 };

	uint cur_text_pos = 0;
	uint i = 0;

	for(std::vector<std::string>::const_iterator it = lines.begin(); it != lines.end(); it++) {
		std::string line = *it;
		if (line.empty())
			line = " ";

		// render this block in a SDL Surface
		SDL_Surface *surface;

		if (!(surface = TTF_RenderUTF8_Shaded(f, line.c_str(),sdl_fg,sdl_bg))) {
			log("Font_Handler::create_static_long_text_surface, an error : %s\n", TTF_GetError());
			log("Text was: %s\n", text.c_str());
			continue; // Ignore this line
		}

		uint new_text_pos = cur_text_pos + line.size();
		if (caret != -1) {
			if (new_text_pos >= caret - i) {
				int caret_line_pos = caret - cur_text_pos - i;
				std::string text_caret_pos = line.substr(0,caret_line_pos);
				render_caret(f,surface,text_caret_pos);
				caret = -1;
			}
			else {
				cur_text_pos = new_text_pos;
			}
			i++;
		}

		m_rendered_lines.push_back(surface);
		global_surface_height += surface->h + line_spacing;
		if( global_surface_width < surface->w)
			global_surface_width = surface->w;
	}

	// blit all this together in one Surface
	return join_sdl_surfaces( global_surface_width, global_surface_height, m_rendered_lines, bg, align, line_spacing);

}

void Font_Handler::render_caret(TTF_Font *f, SDL_Surface *line, const std::string &text_caret_pos) {
	int caret_x,caret_y;

	TTF_SizeUTF8(f, text_caret_pos.c_str(), &caret_x, &caret_y);

	Surface* caret_surf =
		static_cast<const GraphicImpl * const>(g_gr)->get_picture_surface
		(g_gr->get_picture(PicMod_Game, "pics/caret.png"));
	SDL_Surface* caret_surf_sdl = caret_surf->m_surface;

	SDL_Rect r;
	r.x = caret_x - caret_surf_sdl->w;
	r.y = (caret_y - caret_surf_sdl->h) / 2;

	SDL_BlitSurface(caret_surf_sdl, 0, line, &r);
}

/*
* Renders a string into a SDL surface
* Richtext works with this method, because whole richtext content
* is blit into one big surface by the richtext widget itself
*/
SDL_Surface* Font_Handler::draw_string_sdl_surface(std::string font, int size, RGBColor fg, RGBColor bg, std::string text, Align align, int wrap, int style, int line_spacing) {
	TTF_Font* f = m_font_loader->get_font(font,size);
	TTF_SetFontStyle(f,style);
	return create_sdl_text_surface(f,fg,bg,text,align,wrap,line_spacing);
}

/*
* Creates the SDL surface, checks if multiline or not
*/
SDL_Surface* Font_Handler::create_sdl_text_surface(TTF_Font* f, RGBColor fg, RGBColor bg,
        std::string text, Align align, int wrap, int line_spacing) {
	return (wrap > 0  ? create_static_long_text_surface(f, fg, bg, text, align, wrap, line_spacing)
	        : create_single_line_text_surface(f, fg, bg, text, align));
}

//draws richtext, specified by blocks
void Font_Handler::draw_richtext(RenderTarget* dst, RGBColor bg,int dstx, int dsty, std::string text, int wrap, Widget_Cache widget_cache, uint *widget_cache_id) {
	uint picid;
	if (widget_cache == Widget_Cache_Use) {
		//g_gr->get_picture_size(*widget_cache_id,&w,&h);
		picid = *widget_cache_id;
	}
	else {
		if (widget_cache == Widget_Cache_Update) {
			g_gr->free_surface(*widget_cache_id);
		}
		std::vector<Richtext_Block> blocks;
		Text_Parser p;
		p.parse(&text,&blocks,m_varcallback,m_cbdata);

		std::vector<SDL_Surface*> rend_blocks;
		int global_h = 0;

		//Iterate over richtext blocks
		//Seems to be a problem with loading images, and freeing them
		//Refactor to using datastructure
		for (std::vector<Richtext_Block>::iterator richtext_it = blocks.begin();richtext_it != blocks.end();richtext_it++) {
			int cur_line_w = 0;
			int cur_line_h = 0;
			int block_h = 0;

			std::vector<Text_Block> cur_text_blocks = richtext_it->get_text_blocks();
			std::vector<std::string> cur_block_images = richtext_it->get_images();

			std::vector<SDL_Surface*> rend_lines;
			std::vector<SDL_Surface*> rend_cur_words;
			std::vector<SDL_Surface*> rend_cur_images;

			SDL_Surface *block_images = 0;
			int img_surf_h = 0;
			int img_surf_w = 0;

			//First render all images of this richtext block
			for (std::vector<std::string>::iterator img_it = cur_block_images.begin(); img_it != cur_block_images.end(); img_it++) {
				SDL_Rect img_pos;
				img_pos.x = img_surf_w;
				img_pos.y = 0;

				Surface* image =
					static_cast<const GraphicImpl * const>(g_gr)->get_picture_surface
					(g_gr->get_picture(PicMod_Game, img_it->c_str()));
					// Not Font, but game.

				img_surf_h = img_surf_h < static_cast<const int>(image->get_h()) ?
					image->get_h() : img_surf_h;
				img_surf_w = img_surf_w + image->get_w();
				rend_cur_images.push_back(image->m_surface);
			}
			if (rend_cur_images.size()) {
				block_images = join_sdl_surfaces(img_surf_w,img_surf_h,rend_cur_images,bg,Align_Left,0,true,true);
			}

			//Width that's left for text in this richtext block
			int h_space = 3;
			int text_width_left = (wrap - img_surf_w) - h_space;

			//Iterate over text blocks of current richtext block
			for(std::vector<Text_Block>::iterator text_it = cur_text_blocks.begin(); text_it != cur_text_blocks.end(); text_it++) {
				std::vector<std::string> words = text_it->get_words();
				std::vector<uint> line_breaks = text_it->get_line_breaks();

				//Iterate over words of current text block
				uint word_cnt = 0;
				for (std::vector<std::string>::iterator word_it = words.begin(); word_it != words.end(); word_it++) {
					std::string str_word = *word_it;

					int font_style = TTF_STYLE_NORMAL;
					if (text_it->get_font_weight() == "bold")
						font_style |= TTF_STYLE_BOLD;
					if (text_it->get_font_style() == "italic")
						font_style |= TTF_STYLE_ITALIC;
					if (text_it->get_font_decoration() == "underline")
						font_style |= TTF_STYLE_UNDERLINE;

					SDL_Surface *rend_word = draw_string_sdl_surface(text_it->get_font_face(),text_it->get_font_size(),text_it->get_font_color(),bg,
					                         str_word,Align_Left,-1,font_style,text_it->get_line_spacing());

					//is there a break before this word
					//TODO: comparison between signed and unsigned !
					bool break_before = (line_breaks.size() && (line_breaks[0] == word_cnt) ? true : false);

					//Word doesn't fit into current line, or a break was inserted before
					if (((cur_line_w + rend_word->w) > text_width_left) || break_before) {
						SDL_Surface *rend_line = join_sdl_surfaces(cur_line_w,cur_line_h,rend_cur_words,bg,Align_Left,0,true);
						rend_lines.push_back(rend_line);
						block_h+=cur_line_h;
						rend_cur_words.clear();

						//Ignore spaces on begin of the line, if another word follows
						if (str_word != " ") {
							rend_cur_words.push_back(rend_word);
						}

						//Setting line height and width of new word = first in new line
						cur_line_h = rend_word->h;
						cur_line_w = rend_word->w;

						if (break_before) {
							line_breaks.erase(line_breaks.begin());
							//Look for another break at before this word
							while (line_breaks.size()) {
								//TODO: comparison between signed and unsigned !
								if (line_breaks[0] != word_cnt) {
									break;
								}
								else {
									SDL_Surface *space = render_space(*text_it,bg);
									rend_lines.push_back(space);
									block_h+=space->h;
									line_breaks.erase(line_breaks.begin());
								}
							}
						}
					}
					//Word fits regularly in this line
					else {
						rend_cur_words.push_back(rend_word);
						cur_line_w+= rend_word->w;
						cur_line_h = (cur_line_h < rend_word->h ? rend_word->h : cur_line_h);
					}
					//Are there no more words but line breaks left
					bool block_end_breaks = ((word_it + 1) == words.end() && line_breaks.size() ? true : false);
					if (block_end_breaks) {
						if (rend_cur_words.size()) {
							SDL_Surface *rend_line = join_sdl_surfaces(cur_line_w,cur_line_h,rend_cur_words,bg,richtext_it->get_text_align(),0,true);
							rend_lines.push_back(rend_line);
							block_h+=cur_line_h;

							rend_cur_words.clear();
							cur_line_h = 0;
							cur_line_w = 0;
							line_breaks.erase(line_breaks.begin());
						}
						while (line_breaks.size()) {
							SDL_Surface *space = render_space(*text_it,bg);
							rend_lines.push_back(space);
							block_h+=space->h;
							line_breaks.erase(line_breaks.begin());
						}
					}
					word_cnt++;
				}
			}
			//If there are some words left to blit
			if (rend_cur_words.size()) {
				SDL_Surface *rend_line = join_sdl_surfaces(cur_line_w,cur_line_h,rend_cur_words,bg,Align_Left,0,true);
				rend_lines.push_back(rend_line);
				rend_cur_words.clear();
				block_h+=cur_line_h;
			}
			if (!rend_lines.size() && rend_cur_images.size()) {
				rend_lines.push_back(create_empty_sdl_surface(1,1));
			}
			if (rend_lines.size()) {
				int max_x = wrap;

				SDL_Rect img_pos;
				img_pos.x = 0;
				img_pos.y = 0;

				SDL_Rect text_pos;
				text_pos.x = 0;
				text_pos.y = 0;


				if (richtext_it->get_image_align() == Align_Right) {
					img_pos.x = wrap - img_surf_w;
					text_pos.x = 0;
				}
				else if (richtext_it->get_image_align() == Align_HCenter) {
					img_pos.x = (max_x - img_surf_w) / 2;
					text_pos.x = img_pos.x + img_surf_w + h_space;
				}
				else
					text_pos.x = img_surf_w + h_space;

				SDL_Surface* block_surface = create_empty_sdl_surface(wrap,(block_h > img_surf_h ? block_h : img_surf_h));
				SDL_FillRect( block_surface, 0, SDL_MapRGB( block_surface->format,  107,87,55  )); // Set background to colorkey
				SDL_BlitSurface(block_images,0,block_surface,&img_pos);

				{
					SDL_Surface * block_lines = join_sdl_surfaces
						(text_width_left,
						 block_h,
						 rend_lines,
						 bg,
						 richtext_it->get_text_align());
					SDL_BlitSurface(block_lines, 0, block_surface,&text_pos);
					SDL_FreeSurface(block_lines);
				}
				rend_blocks.push_back(block_surface);

				//If image is higher than text, set block height to image height
				block_h = (block_h < img_surf_h ? img_surf_h : block_h);

				global_h+=block_h;

				rend_lines.clear();
			}
		}
		SDL_Surface* global_surface = join_sdl_surfaces(wrap, global_h, rend_blocks, bg);
		picid = convert_sdl_surface(global_surface);
		*widget_cache_id = picid;
	}
	dst->blit(dstx, dsty, picid);
}

SDL_Surface* Font_Handler::render_space(Text_Block &block, RGBColor bg, int style) {
	SDL_Surface *rend_space = 0;
	rend_space = draw_string_sdl_surface(block.get_font_face(),block.get_font_size(),block.get_font_color(),
	                                     bg," ",Align_Left,-1,style,block.get_line_spacing());
	return rend_space;
}

//gets size of picid
void Font_Handler::get_size_from_cache
(const uint widget_cache_id, uint & w, uint & h)
{g_gr->get_picture_size(widget_cache_id, w, h);}

//creates an empty sdl surface of given size
SDL_Surface* Font_Handler::create_empty_sdl_surface(uint w, uint h) {
	SDL_Surface *mask_surf = draw_string_sdl_surface("FreeSans.ttf",10, RGBColor(0,0,0), RGBColor(0,0,0), " ", Align_Left, -1);
	SDL_Surface* surface = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 16,
									mask_surf->format->Rmask,
									mask_surf->format->Gmask,
									mask_surf->format->Bmask,
									mask_surf->format->Amask);
	SDL_FreeSurface(mask_surf);
	return surface;
}

//joins a vectror of surfaces in one big surface
SDL_Surface * Font_Handler::join_sdl_surfaces
(const uint w, const uint h,
 const std::vector<SDL_Surface *> & surfaces,
 const RGBColor bg,
 const Align align,
 const int spacing,
 const bool vertical,
 const bool keep_surfaces)
{
	SDL_Surface * global_surface = create_empty_sdl_surface
		(h ? w : 0, w ? h + spacing * surfaces.size() : 0);
	assert(global_surface);

	SDL_FillRect( global_surface, 0, SDL_MapRGB( global_surface->format, bg.r(), bg.g(), bg.b()));

	int y = 0;
	int x = 0;

	for( uint i = 0; i < surfaces.size(); i++) {
		SDL_Surface* s = surfaces[i];
		SDL_Rect r;

		if (vertical) {
			r.x = x;
			r.y = 0;
		}
		else {
			int alignedX = 0;
			if (align & Align_HCenter) {
				alignedX = (w - s->w) / 2;
			}
			else if (align & Align_Right)
				alignedX += (w - s->w);

			r.x = alignedX;
			r.y = y;
		}
		SDL_BlitSurface(s, 0, global_surface, &r);
		y += s->h + spacing;
		x += s->w + (vertical ? spacing : 0);
		if (!keep_surfaces)
			SDL_FreeSurface( s );
	}
	return global_surface;
}

/*
 * Converts a SDLSurface in a widelands one
 */
uint Font_Handler::convert_sdl_surface( SDL_Surface* surface ) {
	Surface* surf = new Surface();
	SDL_SetColorKey( surface, SDL_SRCCOLORKEY, SDL_MapRGB( surface->format, 107,87,55 ));
	surf->set_sdl_surface( surface );

	uint picid = g_gr->get_picture(PicMod_Font, surf );
	return picid;
}

//Sets dstx and dsty to values for a specified align
void Font_Handler::do_align(Align align, int *dstx, int *dsty, int w, int h) {
	//Vertical Align
	if (align & (Align_VCenter|Align_Bottom)) {
		if (align & Align_VCenter)
			*dsty -= (h+1)/2; // +1 for slight bias to top
		else
			*dsty -= h;
	}

	//Horizontal Align
	if ((align & Align_Horizontal) != Align_Left) {
		if (align & Align_HCenter)
			*dstx -= w/2;
		else if (align & Align_Right)
			*dstx -= w;
	}
}

/*
 * Flushes the cached picture ids
 */
void Font_Handler::flush_cache( void ) {
	while (!m_cache.empty()) {
		g_gr->free_surface (m_cache.front().surface_id);
		m_cache.pop_front();
	}
}
//Deletes widget controlled surface
void Font_Handler::delete_widget_cache(uint widget_cache_id) {
	g_gr->free_surface(widget_cache_id);
}

//Inserts linebreaks into a text, so it doesn't get bigger than max_width when rendered
//Method taken from Wesnoth.
//http://www.wesnoth.org
std::string Font_Handler::word_wrap_text(TTF_Font* f, const std::string &unwrapped_text, int max_width) {
	//std::cerr << "Wrapping word " << unwrapped_text << "\n";

	std::string wrapped_text; // the final result

	size_t word_start_pos = 0;
	std::string cur_word; // including start-whitespace
	std::string cur_line; // the whole line so far

	for(size_t c = 0; c < unwrapped_text.length(); c++) {
		// Find the next word
		bool forced_line_break = false;
		if (c == unwrapped_text.length() - 1) {
			cur_word = unwrapped_text.substr(word_start_pos, c + 1 - word_start_pos);
			word_start_pos = c + 1;
		}
		else if (unwrapped_text[c] == '\n') {
			cur_word = unwrapped_text.substr(word_start_pos, c + 1 - word_start_pos);
			word_start_pos = c + 1;
			forced_line_break = true;
		}
		else if (unwrapped_text[c] == ' ') {
			cur_word = unwrapped_text.substr(word_start_pos, c - word_start_pos);
			word_start_pos = c;
		}
		else {
			continue;
		}

		// Test if the line should be wrapped or not
		std::string tmp_str = cur_line + cur_word;
		if (calc_linewidth(f,tmp_str) > max_width) {
			if (calc_linewidth(f,cur_word) > (max_width /*/ 2*/)) {
				// The last word is too big to fit in a nice way, split it on a char basis
				//std::vector<std::string> split_word = split_utf8_string(cur_word);
				for (uint i=0;i<cur_word.length();i++) {
					tmp_str = cur_line + cur_word[i];
					if (calc_linewidth(f, tmp_str) > max_width) {
						wrapped_text += cur_line + '\n';
						cur_line = "";
					}
					else {
						cur_line += cur_word[i];
					}
				}
			}
			else {
				// Split the line on a word basis
				wrapped_text += cur_line + '\n';
				cur_line = remove_first_space(cur_word);
			}
		}
		else {
			cur_line += cur_word;
		}

		if (forced_line_break) {
			wrapped_text += cur_line;
			cur_line = "";
			forced_line_break = false;
		}
	}

	// Don't forget to add the text left in cur_line
	if (cur_line != "") {
		wrapped_text += cur_line;
	}
	return wrapped_text;
}

std::string Font_Handler::word_wrap_text(std::string font, int size, const std::string &unwrapped_text,int max_width) {
	return word_wrap_text(m_font_loader->get_font(font,size),unwrapped_text,max_width);
}

//removes a leading spacer
//Method taken from Wesnoth.
//http://www.wesnoth.org
std::string Font_Handler::remove_first_space(const std::string &text) {
	if (text.length() > 0 && text[0] == ' ')
		return text.substr(1);
	return text;
}

//calculates size of a given text
void Font_Handler::get_size(std::string font, int size, std::string text, int *w, int *h, int wrap) {
	TTF_Font* f = m_font_loader->get_font(font,size);

	if (wrap > 0)
		text = word_wrap_text(f,text,wrap);
	std::vector<std::string> lines;
	split_string(text, &lines, "\n");

	*w = 0;
	*h = 0;
	for(std::vector<std::string>::const_iterator it = lines.begin(); it != lines.end(); it++) {
		std::string line = *it;
		if (line.empty())
			line = " ";

		int line_w,line_h;
		TTF_SizeUTF8(f, line.c_str(), &line_w, &line_h);

		if (*w < line_w)
			*w = line_w;
		*h+=line_h;
	}
}

//calcultes linewidth of a given text
int Font_Handler::calc_linewidth(TTF_Font* f, std::string &text) {
	int w,h;
	TTF_SizeText(f, text.c_str(), &w, &h);
	return w;
}

/**
 * Registers the variable callback which is used (currently, 11.05)
 * for rendering map variables.
 */
void Font_Handler::register_variable_callback( Varibale_Callback cb, void* cbdata ) {
	m_varcallback=cb;
	m_cbdata=cbdata;
}
void Font_Handler::unregister_variable_callback( ) {
	m_varcallback = 0;
	m_cbdata = 0;
}
