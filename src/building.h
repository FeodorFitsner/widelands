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

#ifndef BUILDING_H
#define BUILDING_H

#include "computer_player_hints.h"
#include "immovable.h"
#include "workarea_info.h"

#include <string>
#include <vector>

class Flag;
class Interactive_Player;
class Tribe_Descr;
class Profile;
namespace UI {struct Window;};
struct EncodeData;

class Building;
class BuildingHints;

/*
 * Common to all buildings!
 */
struct Building_Descr : public Map_Object_Descr {
   friend class Widelands_Map_Buildingdata_Data_Packet;

	struct CostItem {
		std::string name;   // name of ware
		int         amount; // amount

		inline CostItem(const char* iname, int iamount)
			: name(iname), amount(iamount) {}
	};
	typedef std::vector<CostItem> BuildCost;

	Building_Descr(const Tribe_Descr &, const std::string & name);
	virtual ~Building_Descr(void);

	const std::string & name    () const throw () {return m_name;}
	__attribute__ ((deprecated)) const char * get_name() const throw () {return m_name.c_str();}
	const std::string & descname() const throw () {return m_descname;}
	__attribute__ ((deprecated)) const char * get_descname() const throw () {return m_descname.c_str();}
	inline bool get_buildable(void) const { return m_buildable; }
   inline bool get_enhanced_building(void) const { return m_enhanced_building; }
	inline const BuildCost* get_buildcost() const { return &m_buildcost; }
	inline uint get_buildicon() const { return m_buildicon; }
	int get_size() const throw () {return m_size;}
	inline bool get_ismine() const { return m_mine; }
	virtual uint get_ui_anim() const throw () {return get_animation("idle");}

	inline bool get_stopable() const { return m_stopable;}
	const std::string & get_stop_icon() const throw () {return m_stop_icon;}
	const std::string & get_continue_icon() const throw ()
	{return m_continue_icon;}
	const std::vector<char *> & enhances_to() const throw ()
	{return m_enhances_to;}

	Building * create
		(Editor_Game_Base &,
		 Player &,
		 const Coords,
		 const bool construct,
		 const Building_Descr * const = 0)
		const;
	virtual void parse(const char* directory, Profile* prof,
		const EncodeData* encdata);
	virtual void load_graphics();

	virtual uint get_conquers() const {return 0;}
	virtual uint vision_range() const throw () {return get_conquers() + 4;} //  FIXME make this configurable

	const Tribe_Descr & tribe() const throw () {return m_tribe;}
	__attribute__ ((deprecated)) const Tribe_Descr * get_tribe() const throw () {return &m_tribe;}
	Workarea_Info m_workarea_info, m_recursive_workarea_info;

	const BuildingHints & hints() const throw () {return m_hints;}

protected:
	virtual Building * create_object() const = 0;
	Building * create_constructionsite(const Building_Descr * const) const;
	bool         m_stopable;
	std::string  m_stop_icon;
	std::string  m_continue_icon;

private:
	const Tribe_Descr & m_tribe;
	const std::string   m_name;     // internal codename
	std::string         m_descname; // descriptive name for GUI
	bool         m_buildable;       // the player can build this himself
	BuildCost    m_buildcost;
	uint         m_buildicon;       // if buildable: picture in the build dialog
	char*        m_buildicon_fname; // filename for this icon
	int          m_size;            // size of the building
	bool         m_mine;
	std::vector<char*> m_enhances_to;     // building to enhance to or 0
	bool         m_enhanced_building; // if it is one, it is bulldozable
	BuildingHints       m_hints; //  hints (knowledge) for computer players

public:
	static Building_Descr* create_from_dir
		(const Tribe_Descr &,
		 const char * const directory,
		 const EncodeData * const encdata);
};


class Building : public PlayerImmovable {
	friend class Building_Descr;
   friend class Widelands_Map_Buildingdata_Data_Packet;

	MO_DESCR(Building_Descr)

public:
	// Player capabilities: which commands can a player issue for this building?
	enum {
		PCap_Bulldoze = 0, // can bulldoze/remove this buildings
		PCap_Stopable = 1, // is stopable
		PCap_Enhancable = 3, // can be enhanced to something
	};

public:
   enum Type {
      PRODUCTIONSITE=0,
      CONSTRUCTIONSITE,
      MILITARYSITE,
      WAREHOUSE,
		TRAININGSITE
   };

	Building(const Building_Descr &);
	virtual ~Building();

	virtual int get_building_type() const throw () = 0;

	virtual int  get_type    () const throw ();
	virtual int  get_size    () const throw ();
	virtual bool get_passable() const throw ();
   virtual uint get_ui_anim () const;

	virtual Flag* get_base_flag();
	virtual uint get_playercaps() const throw ();
	virtual Coords get_position() const throw () {return m_position;}

	const std::string & name    () const throw () {return descr().name    ();}
	__attribute__ ((deprecated)) const char * get_name    () const throw () {return descr().name().c_str();}
	const std::string & descname() const throw () {return descr().descname();}
	__attribute__ ((deprecated)) const char * get_descname() const throw () {return descr().descname().c_str();}

	virtual const std::string & census_string() const throw ();
	virtual std::string get_statistics_string();

	virtual bool burn_on_destroy();
	virtual void destroy(Editor_Game_Base*);

	void show_options(Interactive_Player* plr);
	void hide_options();

	virtual bool fetch_from_flag(Game* g);
	virtual bool get_building_work(Game* g, Worker* w, bool success) __attribute__ ((noreturn));

	bool leave_check_and_wait(Game* g, Worker* w);
	uint get_conquers() const throw () {return descr().get_conquers();}
	virtual uint vision_range() const throw () {return descr().vision_range();}

	const std::string & get_stop_icon    () const throw ()
	{return descr().get_stop_icon();}
	const std::string & get_continue_icon() const throw ()
	{return descr().get_continue_icon();}
	bool get_stop() const throw () {return m_stop;}
	virtual void set_stop(bool stop);

	const std::vector<char *> & enhances_to() const throw ()
	{return descr().enhances_to();}

   void log_general_info(Editor_Game_Base* egbase);

    // Use on military and training sites
   virtual void drop_soldier(uint) { };
	virtual void soldier_capacity_up   () {}
	virtual void soldier_capacity_down () {}

    // Use on training sites only
   virtual void change_train_priority(uint, int) { };
	virtual void switch_train_mode () { };

   /// Testing stuff
   virtual bool has_soldiers() { return false; };
   virtual void conquered_by (Player*);

	///  Stores the Player_Number of the player who has defeated this building.
	void set_defeating_player(const Player_Number player_number) throw ()
	{m_defeating_player = player_number;}

	void    add_worker(Worker *);
	void remove_worker(Worker *);

protected:
	void start_animation(Editor_Game_Base* g, uint anim);

	virtual void init(Editor_Game_Base* g);
	virtual void cleanup(Editor_Game_Base* g);
	virtual void act(Game* g, uint data);

	virtual void draw
		(const Editor_Game_Base &, RenderTarget &, const FCoords, const Point);
	void draw_help
		(const Editor_Game_Base &, RenderTarget &, const FCoords, const Point);

	virtual UI::Window* create_options_window(Interactive_Player* plr,
		UI::Window** registry) = 0;

	UI::Window* m_optionswindow;
	Coords    m_position;
	Flag*     m_flag;

	uint m_anim;
	int  m_animstart;

	std::vector<Object_Ptr> m_leave_queue; // FIFO queue of workers leaving the building
	uint                    m_leave_time;  // when to wake the next one from leave queue
	Object_Ptr              m_leave_allow; // worker that is allowed to leave now
	bool m_stop;

	//  The player who has defeated this building.
	Player_Number           m_defeating_player;
};

#endif
