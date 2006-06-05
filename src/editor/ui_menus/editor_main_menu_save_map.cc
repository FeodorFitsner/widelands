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

#include <string>
#include <stdio.h>
#include "constants.h"
#include "editor.h"
#include "editorinteractive.h"
#include "editor_main_menu_save_map.h"
#include "editor_main_menu_save_map_make_directory.h"
#include "error.h"
#include "filesystem.h"
#include "i18n.h"
#include "layeredfilesystem.h"
#include "profile.h"
#include "ui_button.h"
#include "ui_editbox.h"
#include "ui_listselect.h"
#include "ui_modal_messagebox.h"
#include "ui_multilinetextarea.h"
#include "ui_textarea.h"
#include "wexception.h"
#include "widelands_map_loader.h"
#include "widelands_map_saver.h"
#include "zip_filesystem.h"

/*
===============
Main_Menu_Save_Map::Main_Menu_Save_Map

Create all the buttons etc...
===============
*/
Main_Menu_Save_Map::Main_Menu_Save_Map(Editor_Interactive *parent)
	: UIWindow(parent, 0, 0, 500, 330, _("Save Map").c_str())
{
   m_parent=parent;

   // Caption
   UITextarea* tt=new UITextarea(this, 0, 0, _("Save Map"), Align_Left);
   tt->set_pos((get_inner_w()-tt->get_w())/2, 5);

   int spacing=5;
   int offsx=spacing;
   int offsy=30;
   int posx=offsx;
   int posy=offsy;

   // listselect
   m_ls=new UIListselect(this, posx, posy, get_inner_w()/2-spacing, get_inner_h()-spacing-offsy-60);
   m_ls->selected.set(this, &Main_Menu_Save_Map::selected);
   m_ls->double_clicked.set(this, &Main_Menu_Save_Map::double_clicked);
   // Filename editbox
   m_editbox=new UIEdit_Box(this, posx, posy+get_inner_h()-spacing-offsy-60+3, get_inner_w()/2-spacing, 20, 1, 0);
   m_editbox->changed.set(this, &Main_Menu_Save_Map::edit_box_changed);

   // the descriptive areas
   // Name
   posx=get_inner_w()/2+spacing;
   posy+=20;
   new UITextarea(this, posx, posy, 150, 20, _("Name: "), Align_CenterLeft);
   m_name=new UITextarea(this, posx+70, posy, 200, 20, "---", Align_CenterLeft);
   posy+=20+spacing;

   // Author
   new UITextarea(this, posx, posy, 150, 20, _("Author: "), Align_CenterLeft);
   m_author=new UITextarea(this, posx+70, posy, 200, 20, "---", Align_CenterLeft);
   posy+=20+spacing;

   // Size
   new UITextarea(this, posx, posy, 70, 20, _("Size: "), Align_CenterLeft);
   m_size=new UITextarea(this, posx+70, posy, 200, 20, "---", Align_CenterLeft);
   posy+=20+spacing;

   // World
   new UITextarea(this, posx, posy, 70, 20, _("World: "), Align_CenterLeft);
   m_world=new UITextarea(this, posx+70, posy, 200, 20, "---", Align_CenterLeft);
   posy+=20+spacing;

   // Players
   new UITextarea(this, posx, posy, 70, 20, _("Players: "), Align_CenterLeft);
   m_nrplayers=new UITextarea(this, posx+70, posy, 200, 20, "---", Align_CenterLeft);
   posy+=20+spacing;


   // Description
   new UITextarea(this, posx, posy, 70, 20, _("Descr: "), Align_CenterLeft);
   m_descr=new UIMultiline_Textarea(this, posx+70, posy, get_inner_w()-posx-spacing-70, get_inner_h()-posy-spacing-40, "---", Align_CenterLeft);


   // Buttons
   posx=5;
   posy=get_inner_h()-30;
   UIButton* but= new UIButton(this, get_inner_w()/2-spacing-80, posy, 80, 20, 0, 1);
   but->clickedid.set(this, &Main_Menu_Save_Map::clicked);
   but->set_title(_("OK").c_str());
   but->set_enabled(false);
   m_ok_btn=but;
   but= new UIButton(this, get_inner_w()/2+spacing, posy, 80, 20, 1, 0);
   but->clickedid.set(this, &Main_Menu_Save_Map::clicked);
   but->set_title(_("Cancel").c_str());
   but= new UIButton(this, spacing, posy, 120, 20, 1, 2);
   but->clickedid.set(this, &Main_Menu_Save_Map::clicked);
   but->set_title(_("Make Directory").c_str());


   m_basedir="maps";
   m_curdir="maps";

   fill_list();

   center_to_parent();
   move_to_top();
}

/*
===============
Main_Menu_Save_Map::~Main_Menu_Save_Map

Unregister from the registry pointer
===============
*/
Main_Menu_Save_Map::~Main_Menu_Save_Map()
{
}

/*
===========
called when the ok button has been clicked
===========
*/
void Main_Menu_Save_Map::clicked(int id) {
   if(id==1) {
      // Ok
      std::string filename=m_editbox->get_text();

      if(filename=="") {
         // Maybe a dir is selected
         filename=static_cast<const char*>(m_ls->get_selection());
      }

      if(g_fs->IsDirectory(filename.c_str()) && !Widelands_Map_Loader::is_widelands_map(filename)) {
         m_curdir=FS_CanonicalizeName(filename);
         m_ls->clear();
         m_mapfiles.clear();
         fill_list();
      } else {
         // Ok, save this map
         if(save_map(filename, ! g_options.pull_section("global")->get_bool("nozip", false)))
            die();
      }
   } else if(id==0) {
      // Cancel
      die();
   } else if(id==2) {
      // Make directory
	   Main_Menu_Save_Map_Make_Directory* md=new Main_Menu_Save_Map_Make_Directory(this, _("unnamed").c_str());
      if(md->run()) {
         g_fs->EnsureDirectoryExists(m_basedir);
         // Create directory
         std::string dirname=md->get_dirname();
         std::string fullname=m_curdir;
         fullname+="/";
         fullname+=dirname;
         g_fs->MakeDirectory(fullname);
         m_ls->clear();
         m_mapfiles.clear();
         fill_list();
      }
      delete md;
   }
}

/*
 * called when a item is selected
 */
void Main_Menu_Save_Map::selected(int i) {
   const char* name=static_cast<const char*>(m_ls->get_selection());

   if(Widelands_Map_Loader::is_widelands_map(name)) {
      Map* map=new Map();
      Map_Loader* m_ml = map->get_correct_loader(name);
      m_ml->preload_map(true); // This has worked before, no problem
      delete m_ml;


      m_editbox->set_text(FS_Filename(name));
      m_ok_btn->set_enabled(true);

      m_name->set_text(map->get_name());
      m_author->set_text(map->get_author());
      m_descr->set_text(map->get_description());
      m_world->set_text(map->get_world_name());

      char buf[200];
      sprintf(buf, "%i", map->get_nrplayers());
      m_nrplayers->set_text(buf);

      sprintf(buf, "%ix%i", map->get_width(), map->get_height());
      m_size->set_text(buf);

      delete map;
   } else {
      m_name->set_text("");
      m_author->set_text("");
      m_descr->set_text("");
      m_world->set_text("");
      m_nrplayers->set_text("");
      m_size->set_text("");
      m_editbox->set_text("");
      m_ok_btn->set_enabled(false);
   }
}

/*
 * An Item has been doubleclicked
 */
void Main_Menu_Save_Map::double_clicked(int) {
   clicked(1);
}

/*
 * fill the file list
 */
void Main_Menu_Save_Map::fill_list(void) {
   // Fill it with all files we find.
   g_fs->FindFiles(m_curdir, "*", &m_mapfiles, 1);

   // First, we add all directorys
   // We manually add the parent directory
   if(m_curdir!=m_basedir) {
      m_parentdir=FS_CanonicalizeName(m_curdir+"/..");
      m_ls->add_entry("<parent>", reinterpret_cast<void*>(const_cast<char*>(m_parentdir.c_str())), false, g_gr->get_picture( PicMod_Game,  "pics/ls_dir.png" ));
   }

   for(filenameset_t::iterator pname = m_mapfiles.begin(); pname != m_mapfiles.end(); pname++) {
      const char *name = pname->c_str();
      if(!strcmp(FS_Filename(name),".")) continue;
      if(!strcmp(FS_Filename(name),"..")) continue; // Upsy, appeared again. ignore
      if(!strcmp(FS_Filename(name),"CVS")) continue;
      if(!g_fs->IsDirectory(name)) continue;
      if(Widelands_Map_Loader::is_widelands_map(name)) continue;

      m_ls->add_entry(FS_Filename(name), reinterpret_cast<void*>(const_cast<char*>(name)), false, g_gr->get_picture( PicMod_Game,  "pics/ls_dir.png" ));
   }

   Map* map=new Map();

   for(filenameset_t::iterator pname = m_mapfiles.begin(); pname != m_mapfiles.end(); pname++) {
      const char *name = pname->c_str();

      Map_Loader* m_ml = map->get_correct_loader(name);
      if(!m_ml) continue;
      if(m_ml->get_type()==Map_Loader::S2ML) continue; // we do not list s2 files since we only write wlmf

      try {
         m_ml->preload_map(true);
         std::string pic="";
         switch(m_ml->get_type()) {
            case Map_Loader::WLML: pic="pics/ls_wlmap.png"; break;
            case Map_Loader::S2ML: pic="pics/ls_s2map.png"; break;
         }
         m_ls->add_entry(FS_Filename(name), reinterpret_cast<void*>(const_cast<char*>(name)), false, g_gr->get_picture( PicMod_Game,  pic.c_str() ));
      } catch(wexception& ) {
         // we simply skip illegal entries
      }
      delete m_ml;

   }
   delete map;

   if(m_ls->get_nr_entries())
      m_ls->select(0);
}

/*
 * The editbox was changed. Enable ok button
 */
void Main_Menu_Save_Map::edit_box_changed(void) {
   m_ok_btn->set_enabled(true);
}

/*
 * Save the map in the current directory with
 * the current filename
 *
 * returns true if dialog should close, false if it
 * should stay open
 */
bool Main_Menu_Save_Map::save_map(std::string filename, bool binary) {
   // Make sure that the base directory exists
   g_fs->EnsureDirectoryExists(m_basedir);

   // ok, first check if the extension matches (ignoring case)
   bool assign_extension=true;
   if(filename.size() >= strlen(WLMF_SUFFIX)) {
      char buffer[10]; // enough for the extension
      filename.copy(buffer, sizeof(WLMF_SUFFIX), filename.size()-strlen(WLMF_SUFFIX));
      if(!strncasecmp(buffer, WLMF_SUFFIX, strlen(WLMF_SUFFIX)))
         assign_extension=false;
   }
   if(assign_extension)
      filename+=WLMF_SUFFIX;

   // Now append directory name
   std::string complete_filename=m_curdir;
   complete_filename+="/";
   complete_filename+=filename;

   // Check if file exists, if it does, show a warning
   if(g_fs->FileExists(complete_filename)) {
      std::string s=_("A File with the name ");
      s+=FS_Filename(filename.c_str());
      s+=_(" exists already. Overwrite?");
      UIModal_Message_Box* mbox= new UIModal_Message_Box(m_parent, _("Save Map Error!!"), s, UIModal_Message_Box::YESNO);
      bool retval=mbox->run();
      delete mbox;
      if(!retval)
         return false;

      // Delete this
      g_fs->Unlink( complete_filename );
   }

   FileSystem* fs = 0;
   if( !binary ) {
   // Make a filesystem out of this
      fs = g_fs->CreateSubFileSystem( complete_filename, FileSystem::FS_DIR );
   } else {
      // Make a zipfile
      fs = g_fs->CreateSubFileSystem( complete_filename, FileSystem::FS_ZIP );
   }
   Widelands_Map_Saver* wms=new Widelands_Map_Saver(fs, m_parent->get_editor());
   try {
      wms->save();
      m_parent->set_need_save(false);
   } catch(std::exception& exe) {
      std::string s=_("Map Saving Error!\nSaved Map-File may be corrupt!\n\nReason given:\n");
      s+=exe.what();
      UIModal_Message_Box* mbox= new UIModal_Message_Box(m_parent, _("Save Map Error!!"), s, UIModal_Message_Box::OK);
      mbox->run();
      delete mbox;
   }
   delete wms;
   delete fs;
   die();

   return true;
}
