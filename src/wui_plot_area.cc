/*
 * Copyright (C) 2002-4 by the Widelands Development Team
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

#include <stdio.h>
#include "constants.h"
#include "font_loader.h"
#include "font_handler.h"
#include "graphic.h"
#include "rendertarget.h"
#include "rgbcolor.h"
#include "ui_panel.h"
#include "wui_plot_area.h"

/*
 * Where to draw tics
 */
static const int how_many_ticks[] = {
   5,  // 15 Mins
   3,  // 30 Mins
   6,  // 1  H
   4,  // 2  H
   4,  // 4  H
   4,  // 8  H
   4,  // 16 H
};

static const int max_x[] = {
   15,
   30,
   60,
   120,
   4,
   8,
   16
};

static const uint time_in_ms[] = {
   15*60*1000,
   30*60*1000,
   1*60*60*1000,
   2*60*60*1000,
   4*60*60*1000,
   8*60*60*1000,
  16*60*60*1000,
};

#define NR_SAMPLES 30   // How many samples per diagramm when relative plotting

#define BG_PIC "pics/plot_area_bg.png"
#define LINE_COLOR RGBColor(0,0,0)

/*
 * Constructor
 */
WUIPlot_Area::WUIPlot_Area(UIPanel* parent, int x, int y, int w, int h) :
   UIPanel(parent, x, y, w, h) {

   m_time = TIME_ONE_HOUR;  // defaults to one hour
   
   m_plotmode = PLOTMODE_ABSOLUTE;
}

/*
 * Destructor
 */
WUIPlot_Area::~WUIPlot_Area( void ) {
}

/*
 * Draw this. This is the main function
 */
void WUIPlot_Area::draw(RenderTarget* dst) {

   // first, tile the background
   dst->tile(0, 0, get_inner_w(), get_inner_h(), g_gr->get_picture(  PicMod_Game, BG_PIC), 0,  0 );

   int spacing = 5;
   int space_at_bottom=15;
   int space_at_right=5;

   float xline_length = get_inner_w()-space_at_right-spacing;
   float yline_length = get_inner_h()-space_at_bottom-spacing;

   // Draw coordinate system
   // X Axis
   dst->draw_line(spacing,get_inner_h()-space_at_bottom,get_inner_w()-space_at_right,get_inner_h()-space_at_bottom, LINE_COLOR);
   // Arrow
   dst->draw_line(spacing,get_inner_h()-space_at_bottom, spacing + 5, get_inner_h()-space_at_bottom-3, LINE_COLOR);
   dst->draw_line(spacing,get_inner_h()-space_at_bottom, spacing + 5, get_inner_h()-space_at_bottom+3, LINE_COLOR);
   // Y Axis
   dst->draw_line(get_inner_w()-space_at_right, spacing, get_inner_w()-space_at_right, get_inner_h()-space_at_bottom, LINE_COLOR);
   // No Arrow here, since this doesn't continue

   // Draw xticks
   float sub = xline_length / how_many_ticks[m_time];
   float posx = get_inner_w()-space_at_right;
   char buf[200];
   for(int i = 0; i <= how_many_ticks[m_time]; i++) {
      dst->draw_line((int)posx, get_inner_h()-space_at_bottom, (int) posx, get_inner_h()-space_at_bottom+3, LINE_COLOR);

      sprintf(buf, "%i", max_x[m_time]/how_many_ticks[m_time] * i);

      int w, h;
      g_fh->get_size(UI_FONT_SMALL, buf, &w, &h, 0);
      g_fh->draw_string(dst, UI_FONT_SMALL, RGBColor(255,0,0), RGBColor(255,255,255), (int) (posx - w/2), get_inner_h()-space_at_bottom+4, buf);
      posx -= sub;
   }

   // draw yticks, one at full, one at half
   dst->draw_line(get_inner_w()-space_at_right, spacing, get_inner_w()-space_at_right-3, spacing, LINE_COLOR);
   dst->draw_line(get_inner_w()-space_at_right, spacing + ((get_inner_h()-space_at_bottom)-spacing)/2, get_inner_w()-space_at_right-3, 
         spacing + ((get_inner_h()-space_at_bottom)-spacing)/2, LINE_COLOR);

   uint max = 0;
   // Find the maximum value
   if( m_plotmode == PLOTMODE_ABSOLUTE )  {
      for(uint i = 0; i < m_plotdata.size(); i++) {
         if(!m_plotdata[i].showplot) continue;
         for(uint l = 0; l < m_plotdata[i].dataset->size(); l++)
            if( max < (*m_plotdata[i].dataset)[l]) 
               max = (*m_plotdata[i].dataset)[l];
      }
   } else {
      for(uint plot = 0; plot < m_plotdata.size(); plot++) {
         if(!m_plotdata[plot].showplot) continue;

         const std::vector<uint>* dataset = m_plotdata[plot].dataset;

         // How many do we take together
         int how_many = (int)( ((float)time_in_ms[m_time] / (float)NR_SAMPLES) /(float)m_sample_rate);

         uint add = 0;
         // Relative data, first entry is always zero
         for(uint i = 0; i < dataset->size(); i++) {
            add += (*dataset)[i];
            if( ! ( (i+1) % how_many ) ) {
               if(add > max) max = add;
               add = 0;
            }
         }
      }
   }

   // Print the maximal value 
   sprintf(buf, "%i", max);
   int w, h;
   g_fh->get_size(UI_FONT_SMALL, buf, &w, &h, 0);
   g_fh->draw_string(dst, UI_FONT_SMALL, RGBColor(120,255,0), RGBColor(255,255,255), get_inner_w()-space_at_right-w-2, spacing, buf);

   // Now, plot the pixels
   sub = xline_length / ((float)time_in_ms[m_time] / (float)m_sample_rate);
   for(uint plot = 0; plot < m_plotdata.size(); plot++) {
      if(!m_plotdata[plot].showplot) continue;

      RGBColor color = m_plotdata[plot].plotcolor;
      const std::vector<uint>* dataset = m_plotdata[plot].dataset;

      std::vector<uint> m_data;
      if( m_plotmode == PLOTMODE_RELATIVE ) {
         // How many do we take together
         int how_many = (int)( ((float)time_in_ms[m_time] / (float)NR_SAMPLES) /(float)m_sample_rate);

         uint add = 0;
         // Relative data, first entry is always zero
         m_data.push_back(0);
         for(uint i = 0; i < dataset->size(); i++) {
            add += (*dataset)[i];
            if( ! ( (i+1) % how_many ) ) {
               m_data.push_back(add);
               add = 0;
            }
         }

         dataset = &m_data;
         sub = xline_length /  (float)NR_SAMPLES;
      }

      posx = get_inner_w()-space_at_right;

      int lx = get_inner_w()-space_at_right; 
      int ly = get_inner_h()-space_at_bottom;
      for(int i = dataset->size()-1; i > 0 && posx > spacing; i--) {
         int value = (*dataset)[i];

         int curx = (int)posx;
         int cury = get_inner_h()-space_at_bottom;
         if(value) { 
            float length_y = yline_length / ((float)max / (float)value);
            cury -= (int)length_y;
         }
         dst->draw_line(lx, ly, curx, cury, color);

         posx -= sub;

         lx = curx; 
         ly = cury;
      }
   } 
}

/*
 * Register a new plot data stream
 */
void WUIPlot_Area::register_plot_data( uint id, const std::vector<uint>* data, RGBColor color ) {
   if(id >= m_plotdata.size()) {
      m_plotdata.resize(id+1);
   }

   m_plotdata[id].dataset = data;
   m_plotdata[id].showplot = false;
   m_plotdata[id].plotcolor = color;
}

/*
 * Show this plot data?
 */
void WUIPlot_Area::show_plot( uint id, bool t ) {
   assert(id < m_plotdata.size());
   m_plotdata[id].showplot = t;
};

/*
 * set time 
 */
void WUIPlot_Area::set_time( int id ) {
   m_time = id;
}

/*
 * Set sample rate the data uses
 */
void WUIPlot_Area::set_sample_rate( uint id ) {
   m_sample_rate = id; 
}

