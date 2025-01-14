/*
 * Copyright (C) 2011-2012 by the Widelands Development Team
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

#ifndef WL_ECONOMY_PORTDOCK_H
#define WL_ECONOMY_PORTDOCK_H

#include <memory>

#include "base/macros.h"
#include "logic/map_objects/immovable.h"
#include "logic/map_objects/tribes/wareworker.h"
#include "economy/shippingitem.h"

namespace Widelands {

struct Fleet;
struct RoutingNodeNeighbour;
struct Ship;
class Warehouse;
class ExpeditionBootstrap;

class PortdockDescr : public MapObjectDescr {
public:
	PortdockDescr(char const* const _name, char const* const _descname);
	~PortdockDescr() override {
	}

private:
	DISALLOW_COPY_AND_ASSIGN(PortdockDescr);
};

/**
 * The PortDock occupies the fields in the water at which ships
 * dock at a port. As such, this class cooperates closely with
 * @ref Warehouse to implement the port functionality.
 *
 * @ref WareInstance and @ref Worker that are waiting to be
 * transported by ship are stored in the PortDock instead of
 * the associated @ref WareHouse.
 *
 * @paragraph PortDockLifetime
 *
 * The PortDock is created and removed by its owning warehouse.
 * Throughout the life of the PortDock, the corresponding @ref Warehouse
 * instance exists.
 *
 * @paragraph Limitations
 *
 * Currently, there is a 1:1 relationship between @ref Warehouse
 * and PortDock. In principle, it would be conceivable to have a
 * port that is on a land bridge and therefore close to two
 * disconnected bodies of water. Such a port would have to have
 * two PortDock that belong to the same @ref Warehouse, but have
 * separate @ref Fleet instances.
 * However, we expect this to be such a rare case that it is not
 * implemented at the moment.
 */
class PortDock : public PlayerImmovable {
public:

	const PortdockDescr& descr() const;

	PortDock(Warehouse* warehouse);
	~PortDock() override;

	void add_position(Widelands::Coords where);
	Warehouse * get_warehouse() const;

	Fleet * get_fleet() const {return m_fleet;}
	PortDock * get_dock(Flag & flag) const;
	bool get_need_ship() const {return m_need_ship || m_expedition_ready;}

	void set_economy(Economy *) override;

	int32_t get_size() const override;
	bool get_passable() const override;

	Flag & base_flag() override;
	PositionList get_positions
		(const EditorGameBase &) const override;
	void draw
		(const EditorGameBase &, RenderTarget &, const FCoords&, const Point&) override;

	void init(EditorGameBase &) override;
	void cleanup(EditorGameBase &) override;

	void add_neighbours(std::vector<RoutingNodeNeighbour> & neighbours);

	void add_shippingitem(Game &, WareInstance &);
	void update_shippingitem(Game &, WareInstance &);

	void add_shippingitem(Game &, Worker &);
	void update_shippingitem(Game &, Worker &);

	void ship_arrived(Game &, Ship &);

	void log_general_info(const EditorGameBase &) override;

	uint32_t count_waiting(WareWorker waretype, DescriptionIndex wareindex);
	uint32_t count_waiting();

	// Returns true if a expedition is started or ready to be send out.
	bool expedition_started();

	// Called when the button in the warehouse window is pressed.
	void start_expedition();
	void cancel_expedition(Game &);

	// May return nullptr when there is no expedition ongoing or if the
	// expedition ship is already underway.
	ExpeditionBootstrap* expedition_bootstrap();

	// Gets called by the ExpeditionBootstrap as soon as all wares and workers are available.
	void expedition_bootstrap_complete(Game& game);

private:
	friend struct Fleet;

	void init_fleet(EditorGameBase & egbase);
	void set_fleet(Fleet * fleet);
	void _update_shippingitem(Game &, std::vector<ShippingItem>::iterator);
	void set_need_ship(Game &, bool need);

	Fleet * m_fleet;
	Warehouse * m_warehouse;
	PositionList m_dockpoints;
	std::vector<ShippingItem> m_waiting;
	bool m_need_ship;
	bool m_expedition_ready;

	std::unique_ptr<ExpeditionBootstrap> m_expedition_bootstrap;

	// saving and loading
protected:
	class Loader : public PlayerImmovable::Loader {
	public:
		Loader();

		void load(FileRead &);
		void load_pointers() override;
		void load_finish() override;

	private:
		uint32_t m_warehouse;
		std::vector<ShippingItem::Loader> m_waiting;
	};

public:
	bool has_new_save_support() override {return true;}
	void save(EditorGameBase &, MapObjectSaver &, FileWrite &) override;

	static MapObject::Loader * load
		(EditorGameBase &, MapObjectLoader &, FileRead &);
};

extern PortdockDescr g_portdock_descr;

} // namespace Widelands

#endif  // end of include guard: WL_ECONOMY_PORTDOCK_H
