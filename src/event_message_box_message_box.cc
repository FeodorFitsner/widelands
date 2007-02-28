/*
 * Copyright (C) 2002-2004, 2006-2007 by the Widelands Development Team
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

#include "event_message_box_message_box.h"
#include "event_message_box.h"
#include "game.h"
#include "graphic.h"
#include "editorinteractive.h"
#include "ui_multilinetextarea.h"
#include "ui_textarea.h"
#include "ui_button.h"
#include "constants.h"
#include "trigger_null.h"
#include "util.h"

/*
 * The message box himself
 */
Message_Box_Event_Message_Box::Message_Box_Event_Message_Box(Game* game, Event_Message_Box* event,
      int gposx, int gposy, int w, int h) :
UI::Window(game->get_iabase(), 0, 0, 600, 400, event->get_window_title() ) {

   m_game = game;

   UI::Multiline_Textarea* m_text=0;
   int spacing=5;
   int offsy=5;
   int offsx=spacing;
   int posx=offsx;
   int posy=offsy;

   set_inner_size(w,h);
   m_text=new UI::Multiline_Textarea(this, posx, posy, get_inner_w()-posx-spacing, get_inner_h()-posy-2*spacing-50, "", Align_Left);

   if(m_text)
      m_text->set_text( event->get_text());

   // Buttons
   int but_width=80;
   int space=get_inner_w()-2*spacing;
   space-=but_width*event->get_nr_buttons();
   space/=event->get_nr_buttons()+1;
   posx=spacing;
   posy=get_inner_h()-30;
   m_trigger.resize(event->get_nr_buttons());
   for(int i=0; i<event->get_nr_buttons(); i++) {
      posx+=space;
		new UI::IDButton<Message_Box_Event_Message_Box, int>
			(this,
			 posx, posy, but_width, 20,
			 0,
			 &Message_Box_Event_Message_Box::clicked, this, i,
			 event->get_button_name(i));
      posx+=but_width;
      m_trigger[i]=event->get_button_trigger(i);
   }

   m_is_modal = event->get_is_modal();

   center_to_parent();

	if (gposx != -1) set_pos(Point(gposy, get_y()));
	if (gposy != -1) set_pos(Point(get_x(), gposy));
}


/*
 * Handle mouse button events
 *
 * we might be a modal, therefore, if we are, we delete ourself through end_modal()
 * otherwise through die()
 * We are not closable by right clicking so that we are not closed by accidental
 * scrolling.
 * We are not draggable.
 */
bool Message_Box_Event_Message_Box::handle_mousepress(const Uint8 btn, int, int) {
	if (btn == SDL_BUTTON_RIGHT) {play_click(); end_modal(0); return true;}
	return false;
}
bool Message_Box_Event_Message_Box::handle_mouserelease(const Uint8, int, int)
{return false;}

/*
 * clicked
 */
void Message_Box_Event_Message_Box::clicked(int i) {
   if(i==-1) {
      // we should end this dialog
      if(m_is_modal) {
         end_modal(0);
         return;
      } else {
         die();
         return;
      }
   } else {
      // One of the buttons has been pressed
//      NoLog("Button %i has been pressed, nr of buttons: %i!\n", i, event->get_nr_buttons());
      Trigger_Null* t=m_trigger[i];
      if(t) {
         t->set_trigger_manually(true);
         t->check_set_conditions( m_game ); // forcefully update this trigger
      }
      clicked(-1);
      return;
   }
}
