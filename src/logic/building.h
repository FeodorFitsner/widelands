/*
 * Copyright (C) 2002-2004, 2006-2013 by the Widelands Development Team
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

#ifndef BUILDING_H
#define BUILDING_H

#include <cstring>
#include <string>
#include <vector>

#include <boost/signals2.hpp>

#include "ai/ai_hints.h"
#include "logic/buildcost.h"
#include "logic/immovable.h"
#include "io/filewrite.h"
#include "logic/soldier_counts.h"
#include "logic/ware_types.h"
#include "logic/widelands.h"
#include "workarea_info.h"
#include "writeHTML.h"

namespace UI {class Window;}
struct BuildingHints;
class Interactive_GameBase;
struct Profile;
class Image;

namespace Widelands {

struct Flag;
struct Message;
struct Tribe_Descr;
struct WaresQueue;

class Building;

#define LOW_PRIORITY             2
#define DEFAULT_PRIORITY         4
#define HIGH_PRIORITY            8

/*
 * Common to all buildings!
 */
struct Building_Descr : public Map_Object_Descr {
	typedef std::set<Building_Index> Enhancements;
	typedef std::vector<const Building_Descr*> FormerBuildings;

	Building_Descr
		(char const * _name, char const * _descname,
		 const std::string & directory, Profile &, Section & global_s,
		 const Tribe_Descr &);

	bool is_buildable   () const {return m_buildable;}
	bool is_destructible() const {return m_destructible;}
	bool is_enhanced    () const {return m_enhanced_building;}
	bool global() const {return m_global;}

	/**
	 * The build cost for direct construction
	 */
	const Buildcost & buildcost() const throw () {return m_buildcost;}

	/**
	 * Returned wares for dismantling
	 */
	const Buildcost & returned_wares() const throw () {return m_return_dismantle;}

	/**
	 * The build cost for enhancing a previous building
	 */
	const Buildcost & enhancement_cost() const throw () {return m_enhance_cost;}

	/**
	 * The returned wares for a enhaced building
	 */
	const Buildcost & returned_wares_enhanced() const throw () {return m_return_enhanced;}
	const Image* get_buildicon() const {return m_buildicon;}
	int32_t get_size() const throw () {return m_size;}
	bool get_ismine() const {return m_mine;}
	bool get_isport() const {return m_port;}
	virtual uint32_t get_ui_anim() const {return get_animation("idle");}

	const Enhancements & enhancements() const throw () {return m_enhancements;}
	void add_enhancement(const Building_Index & i) {
		assert(not m_enhancements.count(i));
		m_enhancements.insert(i);
	}

	/// Create a building of this type in the game. Calls init, which does
	/// different things for different types of buildings (such as conquering
	/// land and requesting things). Therefore this must not be used to allocate
	/// a building during savegame loading. (It would cause many bugs.)
	///
	/// Does not perform any sanity checks.
	/// If former_buildings is not empty this is an enhancing.
	Building & create
		(Editor_Game_Base &,
		 Player &,
		 Coords,
		 bool                   construct,
		 bool                   loading = false,
		 FormerBuildings former_buildings = FormerBuildings())
		const;
#ifdef WRITE_GAME_DATA_AS_HTML
	void writeHTML(::FileWrite &) const;
#endif
	virtual void load_graphics();

	virtual uint32_t get_conquers() const;
	virtual uint32_t vision_range() const throw ();
	bool has_help_text() const {return m_helptext_script != "";}
	std::string helptext_script() const {return m_helptext_script;}

	const Tribe_Descr & tribe() const throw () {return m_tribe;}
	Workarea_Info m_workarea_info;

	virtual int32_t suitability(const Map &, FCoords) const;
	const BuildingHints & hints() const {return m_hints;}

protected:
	virtual Building & create_object() const = 0;
	Building & create_constructionsite() const;

private:
	const Tribe_Descr & m_tribe;
	bool          m_buildable;       // the player can build this himself
	bool          m_destructible;    // the player can destruct this himself
	Buildcost     m_buildcost;
	Buildcost     m_return_dismantle; // Returned wares on dismantle
	Buildcost     m_enhance_cost;     // cost for enhancing
	Buildcost     m_return_enhanced;   // Returned ware for dismantling an enhanced building
	const Image*     m_buildicon;       // if buildable: picture in the build dialog
	std::string   m_buildicon_fname; // filename for this icon
	int32_t       m_size;            // size of the building
	bool          m_mine;
	bool          m_port;
	Enhancements  m_enhancements;
	bool          m_enhanced_building; // if it is one, it is bulldozable
	BuildingHints m_hints;             // hints (knowledge) for computer players
	bool          m_global;            // whether this is a "global" building
	std::string   m_helptext_script;

	// for migration, 0 is the default, meaning get_conquers() + 4
	uint32_t m_vision_range;
};


class Building : public PlayerImmovable {
	friend struct Building_Descr;
	friend struct Map_Buildingdata_Data_Packet;

	MO_DESCR(Building_Descr)

public:
	// Player capabilities: which commands can a player issue for this building?
	enum {
		PCap_Bulldoze = 1, // can bulldoze/remove this buildings
		PCap_Dismantle = 1 << 1, // can dismantle this buildings
		PCap_Enhancable = 1 << 2, // can be enhanced to something
	};

	typedef std::vector<const Building_Descr*> FormerBuildings;

public:
	Building(const Building_Descr &);
	virtual ~Building();

	void load_finish(Editor_Game_Base &);

	const Tribe_Descr & tribe() const throw () {return descr().tribe();}

	virtual int32_t  get_type    () const throw ();
	char const * type_name() const throw () {return "building";}
	virtual int32_t  get_size    () const throw ();
	virtual bool get_passable() const throw ();
	virtual uint32_t get_ui_anim () const;

	virtual Flag & base_flag();
	virtual uint32_t get_playercaps() const throw ();

	virtual Coords get_position() const throw () {return m_position;}
	virtual PositionList get_positions (const Editor_Game_Base &) const throw ();

	const std::string & name() const throw ();
	const std::string & descname() const throw () {return descr().descname();}

	std::string info_string(const std::string & format);
	virtual std::string get_statistics_string();

	/// \returns the queue for a ware type or \throws _wexception.
	virtual WaresQueue & waresqueue(Ware_Index);

	virtual bool burn_on_destroy();
	virtual void destroy(Editor_Game_Base &);

	void show_options(Interactive_GameBase &, bool avoid_fastclick = false, Point pos = Point(- 1, - 1));
	void hide_options();
	void refresh_options(Interactive_GameBase &);

	virtual bool fetch_from_flag(Game &);
	virtual bool get_building_work(Game &, Worker &, bool success);

	bool leave_check_and_wait(Game &, Worker &);
	void leave_skip(Game &, Worker &);
	uint32_t get_conquers() const throw () {return descr().get_conquers();}
	virtual uint32_t vision_range() const throw () {
		return descr().vision_range();
	}

	int32_t get_base_priority() const {return m_priority;}
	int32_t get_priority
		(int32_t type, Ware_Index, bool adjust = true) const;
	void set_priority(int32_t new_priority);
	void set_priority(int32_t type, Ware_Index ware_index, int32_t new_priority);

	void collect_priorities
		(std::map<int32_t, std::map<Ware_Index, int32_t> > & p) const;

	const std::set<Building_Index> & enhancements() const throw () {
		return descr().enhancements();
	}

	/**
	 * The former buildings vector keeps track of all former buildings
	 * that have been enhanced up to the current one. The current building
	 * descr will be in the last position. For construction sites, it is
	 * empty except if a former building is being enhanced. For a dismantle
	 * site, the last item will be the one being dismantled.
	 */
	const FormerBuildings get_former_buildings() {
		return m_old_buildings;
	}

	virtual void log_general_info(const Editor_Game_Base &);

	//  Use on training sites only.
	virtual void change_train_priority(uint32_t, int32_t) {};
	virtual void switch_train_mode () {};

	///  Stores the Player_Number of the player who has defeated this building.
	void set_defeating_player(Player_Number const player_number) {
		m_defeating_player = player_number;
	}

	void    add_worker(Worker &);
	void remove_worker(Worker &);
	mutable boost::signals2::signal<void ()> workers_changed;

	void send_message
		(Game & game,
		 const std::string & msgsender,
		 const std::string & title,
		 const std::string & description,
		 uint32_t throttle_time = 0,
		 uint32_t throttle_radius = 0);
protected:
	void start_animation(Editor_Game_Base &, uint32_t anim);

	virtual void init(Editor_Game_Base &);
	virtual void cleanup(Editor_Game_Base &);
	virtual void act(Game &, uint32_t data);

	virtual void draw(const Editor_Game_Base &, RenderTarget &, const FCoords&, const Point&);
	void draw_help(const Editor_Game_Base &, RenderTarget &, const FCoords&, const Point&);

	virtual void create_options_window
		(Interactive_GameBase &, UI::Window * & registry)
		= 0;

	void set_seeing(bool see);

	UI::Window * m_optionswindow;
	Coords       m_position;
	Flag       * m_flag;

	uint32_t m_anim;
	int32_t  m_animstart;

	typedef std::vector<OPtr<Worker> > Leave_Queue;
	Leave_Queue m_leave_queue; //  FIFO queue of workers leaving the building
	uint32_t    m_leave_time;  //  when to wake the next one from leave queue
	Object_Ptr  m_leave_allow; //  worker that is allowed to leave now

	//  The player who has defeated this building.
	Player_Number           m_defeating_player;

	int32_t m_priority; // base priority
	std::map<Ware_Index, int32_t> m_ware_priorities;

	/// Whether we see our vision_range area based on workers in the building
	bool m_seeing;

	// Signals connected for the option window
	std::vector<boost::signals2::connection> options_window_connections;

	// The former buildings descrs, with the current one in last position.
	FormerBuildings m_old_buildings;
};

}

#endif
