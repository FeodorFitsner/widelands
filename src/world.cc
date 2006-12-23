/*
 * Copyright (C) 2002, 2004, 2006 by the Widelands Development Team
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

#include <iostream>
#include <sstream>
#include "constants.h"
#include "error.h"
#include "fileread.h"
#include "i18n.h"
#include "layered_filesystem.h"
#include "graphic.h"
#include "profile.h"
#include "util.h"
#include "wexception.h"
#include "world.h"
#include "worlddata.h"

using std::cerr;
using std::endl;

/*
=============================================================================

Resource_Descr

=============================================================================
*/

/*
==============
Resource_Descr::parse

Parse a resource description section.
==============
*/
void Resource_Descr::parse(Section *s, std::string basedir)
{
   const char* string;


   m_name=s->get_name();
   m_descrname = s->get_string("name", s->get_name());
   m_is_detectable=s->get_bool("detectable", true);

   m_max_amount = s->get_safe_int("max_amount");
   while(s->get_next_string("editor_pic", &string))
   {
      std::vector<std::string> args;
      Editor_Pic i;

      split_string(string, &args, " \t");

      if (args.size() != 1 && args.size() != 2)
      {
         log("Resource '%s' has bad editor_pic=%s\n", m_name.c_str(), string);
         continue;
      }

      i.picname = basedir + "/pics/";
      i.picname += args[0];
      i.upperlimit = -1;

      if (args.size() >= 2)
      {
         char* endp;

         i.upperlimit = strtol(args[1].c_str(), &endp, 0);

         if (endp && *endp)
         {
            log("Resource '%s' has bad editor_pic=%s\n", m_name.c_str(), string);
            continue;
         }
      }

      m_editor_pics.push_back(i);
   }
   if(!m_editor_pics.size())
      throw wexception("Resource '%s' has no editor_pic", m_name.c_str());
}


/*
 * Get the correct editor pic for this amount of this resource
 */
std::string Resource_Descr::get_editor_pic(uint amount) {
	uint bestmatch = 0;

	assert(m_editor_pics.size());

	for(uint i = 1; i < m_editor_pics.size(); ++i)
	{
		int diff1 = m_editor_pics[bestmatch].upperlimit - (int)amount;
		int diff2 = m_editor_pics[i].upperlimit - (int)amount;

		// This is a catch-all for high amounts
		if (m_editor_pics[i].upperlimit < 0)
		{
			if (diff1 < 0) {
				bestmatch = i;
				continue;
			}

			continue;
		}

		// This is lower than the actual amount
		if (diff2 < 0)
		{
			if (m_editor_pics[bestmatch].upperlimit < 0)
				continue;

			if (diff1 < diff2) {
				bestmatch = i; // still better than previous best match
				continue;
			}

			continue;
		}

		// This is higher than the actual amount
		if (m_editor_pics[bestmatch].upperlimit < 0 || diff1 > diff2 || diff1 < 0) {
			bestmatch = i;
			continue;
		}
	}

	//noLog("Resource(%s): Editor_Pic '%s' for amount = %u\n",
	//m_name.c_str(), m_editor_pics[bestmatch].picname.c_str(), amount);

	return m_editor_pics[bestmatch].picname;
}


/*
=============================================================================

World

=============================================================================
*/

World::World(const char* name)
{
	char directory[256];

	try
	{
      // Grab the localisation text domain
      sprintf( directory, "world_%s", name );
      i18n::grab_textdomain( directory );

      snprintf(directory, sizeof(directory), "worlds/%s", name);
		m_basedir = directory;

		parse_root_conf(name);
		parse_resources();
		parse_terrains();
		parse_bobs();

		i18n::release_textdomain();
	}
	catch(std::exception &e)
	{
		// tag with world name
		throw wexception("Error loading world %s: %s", name, e.what());
	}
}


/*
===============
World::postload

Load all logic game data now
===============
*/
void World::postload(Editor_Game_Base *) {
	// TODO: move more loads to postload
}


/*
===============
World::load_graphics

Load graphics data here
===============
*/
void World::load_graphics()
{
	int i;

	// Load terrain graphics
	for(i = 0; i < ters.get_nitems(); i++)
		ters.get(i)->load_graphics();

	// TODO: load more graphics
}


//
// down here: Private functions for loading
//

//
// read the <world-directory>/conf
//
void World::parse_root_conf(const char *name)
{
	char fname[256];

	snprintf(fname, sizeof(fname), "%s/conf", m_basedir.c_str());

	try
	{
		Profile prof(fname);
		Section* s;

		s = prof.get_safe_section("world");

		const char* str;

		str = s->get_string("name", name);
		snprintf(hd.name, sizeof(hd.name), "%s", str);

		str = s->get_safe_string("author");
		snprintf(hd.author, sizeof(hd.author), "%s", str);

		str = s->get_safe_string("descr");
		snprintf(hd.descr, sizeof(hd.descr), "%s", str);

		prof.check_used();
	}
	catch(std::exception &e) {
		throw wexception("%s: %s", fname, e.what());
	}
}

void World::parse_resources()
{
	char fname[256];

	snprintf(fname, sizeof(fname), "%s/resconf", m_basedir.c_str());

	try
	{
		Profile prof(fname);
      Section* section;

      Resource_Descr* descr;
      while((section=prof.get_next_section(0))) {
         descr=new Resource_Descr();
         descr->parse(section,m_basedir);
         m_resources.add(descr);
      }
	}
	catch(std::exception &e) {
		throw wexception("%s: %s", fname, e.what());
	}
}

void World::parse_terrains()
{
	char fname[256];

	snprintf(fname, sizeof(fname), "%s/terrainconf", m_basedir.c_str());

	try
	{
		Profile prof(fname);
		Section* s;

		while((s = prof.get_next_section(0)))
		{
			Terrain_Descr *ter = new Terrain_Descr(m_basedir.c_str(), s, &m_resources);
			ters.add(ter);
		}

		prof.check_used();
	}
	catch(std::exception &e) {
		throw wexception("%s: %s", fname, e.what());
	}
}

void World::parse_bobs()
{
	char subdir[256];
	filenameset_t dirs;

	snprintf(subdir, sizeof(subdir), "%s/bobs", m_basedir.c_str());

	g_fs->FindFiles(subdir, "*", &dirs);

	for(filenameset_t::iterator it = dirs.begin(); it != dirs.end(); it++) {
		char fname[256];

		snprintf(fname, sizeof(fname), "%s/conf", it->c_str());

		if (!g_fs->FileExists(fname))
			continue;

		const char *name;
		const char *slash = strrchr(it->c_str(), '/');
		const char *backslash = strrchr(it->c_str(), '\\');

		if (backslash && (!slash || backslash > slash))
			slash = backslash;

		if (slash)
			name = slash+1;
		else
			name = it->c_str();

		try
		{
			Profile prof(fname, "global"); // section-less file
			Section *s = prof.get_safe_section("global");
			const char *type = s->get_safe_string("type");

			if (!strcasecmp(type, "critter")) {
				Bob_Descr *descr;
				descr = Bob_Descr::create_from_dir(name, it->c_str(), &prof, 0);
				bobs.add(descr);
			} else {
				Immovable_Descr *descr = new Immovable_Descr(name, 0);
				descr->parse(it->c_str(), &prof);
				immovables.add(descr);
			}
		} catch(std::exception &e) {
			cerr << it->c_str() << ": " << e.what() << " (garbage directory?)" << endl;
		} catch(...) {
			cerr << it->c_str() << ": unknown exception (garbage directory?)" << endl;
		}
	}
}

/*
 * World::exists_world()
 */
bool World::exists_world(std::string worldname) {
   std::string buf;
   buf="worlds/" + worldname + "/conf";;

   FileRead f;
	return f.TryOpen(*g_fs, buf.c_str());
}

/*
 * World::get_all_worlds()
 */
void World::get_all_worlds(std::vector<std::string>* retval) {
   retval->resize(0);

   // get all worlds
   filenameset_t m_worlds;
   g_fs->FindFiles("worlds", "*", &m_worlds);
   for(filenameset_t::iterator pname = m_worlds.begin(); pname != m_worlds.end(); pname++) {
      std::string world=*pname;
      world.erase(0,7); // remove worlds/
      if(World::exists_world(world.c_str()))
         retval->push_back(world);
   }
}


/*
==============================================================================

Terrain_Descr

==============================================================================
*/

Terrain_Descr::Terrain_Descr(const char* directory, Section* s, Descr_Maintainer<Resource_Descr>* resources)
:
m_texture           (0),
m_frametime         (FRAME_LENGTH),
m_picnametempl      (0),
m_valid_resources   (0),
m_nr_valid_resources(0),
m_default_resources (-1),
m_default_amount    (0)
{

	// Read configuration
	snprintf(m_name, sizeof(m_name), "%s", s->get_name());

	// Parse the default resource
	const char * str = s->get_string("def_resources", 0);
	if (str) {
      std::istringstream str1(str);
      std::string resource;
      int amount;
	   str1 >> resource >> amount;
      int res=resources->get_index(resource.c_str());;
      if(res==-1)
         throw wexception("Terrain %s has valid resource %s which doesn't exist in world!\n", s->get_name(), resource.c_str());
      m_default_resources=res;
      m_default_amount=amount;
   }

   // Parse valid resources
   std::string str1=s->get_string("resources", "");
   if(str1!="") {
      int nres=1;
      uint i=0;
      while(i < str1.size()) { if(str1[i]==',') { nres++; }  i++; }

      m_nr_valid_resources=nres;
      if(nres==1)
         m_valid_resources=new uchar;
      else
         m_valid_resources=new uchar[nres];
      std::string curres;
      i=0;
      int cur_res=0;
      while(i<=str1.size()) {
         if(str1[i] == ' ' || str1[i] == ' ' || str1[i]=='\t') { ++i; continue; }
         if(str1[i]==',' || i==str1.size()) {
            int res=resources->get_index(curres.c_str());;
            if(res==-1)
               throw wexception("Terrain %s has valid resource %s which doesn't exist in world!\n", s->get_name(), curres.c_str());
            m_valid_resources[cur_res++]=res;
            curres="";
         } else {
            curres.append(1, str1[i]);
         }
         i++;
      }
   }

	int fps = s->get_int("fps");
	if (fps > 0)
		m_frametime = 1000 / fps;

	// switch is
	str = s->get_safe_string("is");

	if(!strcasecmp(str, "dry")) {
		m_is = TERRAIN_DRY;
	} else if(!strcasecmp(str, "green")) {
		m_is = 0;
	} else if(!strcasecmp(str, "water")) {
		m_is = TERRAIN_WATER|TERRAIN_DRY|TERRAIN_UNPASSABLE;
	} else if(!strcasecmp(str, "acid")) {
		m_is = TERRAIN_ACID|TERRAIN_DRY|TERRAIN_UNPASSABLE;
	} else if(!strcasecmp(str, "mountain")) {
		m_is = TERRAIN_DRY|TERRAIN_MOUNTAIN;
	} else if(!strcasecmp(str, "dead")) {
		m_is = TERRAIN_DRY|TERRAIN_UNPASSABLE|TERRAIN_ACID;
	} else if(!strcasecmp(str, "unpassable")) {
		m_is = TERRAIN_DRY|TERRAIN_UNPASSABLE;
	} else
		throw wexception("%s: invalid type '%s'", m_name, str);

	// Determine template of the texture animation pictures
	char fnametmpl[256];

	str = s->get_string("texture", 0);
	if (str)
		snprintf(fnametmpl, sizeof(fnametmpl), "%s/%s", directory, str);
	else
		snprintf(fnametmpl, sizeof(fnametmpl), "%s/pics/%s_??.png", directory, m_name);

	m_picnametempl = strdup(fnametmpl);
}

Terrain_Descr::~Terrain_Descr()
{
	if (m_picnametempl)
		free(m_picnametempl);
   if(m_nr_valid_resources==1) {
      delete m_valid_resources;
   }
   if(m_nr_valid_resources>1) {
      delete[] m_valid_resources;
   }
   m_nr_valid_resources=0;
   m_valid_resources=0;
}


/*
===============
Terrain_Descr::load_graphics

Trigger load of the actual animation frames.
===============
*/
void Terrain_Descr::load_graphics()
{
	if (m_picnametempl)
		m_texture = g_gr->get_maptexture(m_picnametempl, m_frametime);
}
