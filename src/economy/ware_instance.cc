/*
 * Copyright (C) 2004, 2006-2009 by the Widelands Development Team
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

#include "ware_instance.h"

// Package includes
#include "economy.h"
#include "flag.h"
#include "transfer.h"

#include "logic/game.h"
#include "logic/tribe.h"
#include "logic/warehouse.h"
#include "logic/worker.h"
#include "map_io/widelands_map_map_object_loader.h"
#include "map_io/widelands_map_map_object_saver.h"
#include "request.h"
#include "wexception.h"
#include "upcast.h"

namespace Widelands {


/**
 * Whenever a WareInstance is idle, it issues an IdleWareSupply.
 *
 * \todo This maybe shouldn't be here.
*/
struct IdleWareSupply : public Supply {
	IdleWareSupply(WareInstance &);
	virtual ~IdleWareSupply();

	void set_economy(Economy *);

	//  implementation of Supply
	virtual PlayerImmovable * get_position(Game &);
	virtual bool is_active() const throw ();
	virtual bool has_storage() const throw ();
	virtual void get_ware_type(bool& isworker, Ware_Index& ware) const;
	virtual void send_to_storage(Game &, Warehouse* wh);

	virtual uint32_t nr_supplies(Game const &, Request const &) const;
	virtual WareInstance & launch_item(Game &, Request const &);
	virtual Worker & launch_worker(Game &, Request const &)
		__attribute__ ((noreturn));

private:
	WareInstance & m_ware;
	Economy      * m_economy;
};

/**
 * Initialize the Supply and update the economy.
*/
IdleWareSupply::IdleWareSupply(WareInstance & ware) :
	m_ware(ware), m_economy(0)
{
	set_economy(ware.get_economy());
}

/**
 * Cleanup.
*/
IdleWareSupply::~IdleWareSupply()
{
	set_economy(0);
}

/**
 * Add/remove self from economies as necessary.
*/
void IdleWareSupply::set_economy(Economy * const e)
{
	if (m_economy != e) {
		if (m_economy)
			m_economy->remove_supply(*this);
		if ((m_economy = e))
			m_economy->   add_supply(*this);
	}
}

/**
 * Figure out the player immovable that this ware belongs to.
*/
PlayerImmovable * IdleWareSupply::get_position(Game & game)
{
	Map_Object * const loc = m_ware.get_location(game);

	if (upcast(PlayerImmovable, playerimmovable, loc))
		return playerimmovable;

	if (upcast(Worker, worker, loc))
		return worker->get_location(game);

	return 0;
}

bool IdleWareSupply::is_active() const throw ()
{
	return true;
}

bool IdleWareSupply::has_storage()  const throw ()
{
	return m_ware.is_moving();
}

void IdleWareSupply::get_ware_type(bool& isworker, Ware_Index& ware) const
{
	isworker = false;
	ware = m_ware.descr_index();
}

uint32_t IdleWareSupply::nr_supplies(Game const &, Request const & req) const
{
	if
		(req.get_type() == Request::WARE &&
		 req.get_index() == m_ware.descr_index())
		return 1;

	return 0;
}

/**
 * The item is already "launched", so we only need to return it.
*/
WareInstance & IdleWareSupply::launch_item(Game &, Request const & req) {
	if (req.get_type() != Request::WARE)
		throw wexception
			("IdleWareSupply::launch_item : called for non-item request");
	if (req.get_index() != m_ware.descr_index())
		throw wexception
			("IdleWareSupply: ware(%u) (type = %i) requested for %i",
			 m_ware.serial(),
			 m_ware.descr_index().value(),
			 req.get_index().value());

	return m_ware;
}

Worker & IdleWareSupply::launch_worker(Game &, Request const &)
{
	throw wexception("IdleWareSupply::launch_worker makes no sense");
}

void IdleWareSupply::send_to_storage(Game & game, Warehouse* wh)
{
	assert(!has_storage());

	Transfer * t = new Transfer(game, m_ware);
	t->set_destination(*wh);
	m_ware.set_transfer(game, *t);
}


/*************************************************************************/
/*                     Ware Instance Implementation                      */
/*************************************************************************/
WareInstance::WareInstance
	(Ware_Index const i, const Item_Ware_Descr * const ware_descr)
:
Map_Object   (ware_descr),
m_economy    (0),
m_descr_index(i),
m_supply     (0),
m_transfer   (0)
{}

WareInstance::~WareInstance()
{
	if (m_supply) {
		molog("Ware %u still has supply %p\n", m_descr_index.value(), m_supply);
		delete m_supply;
	}
}

int32_t WareInstance::get_type() const throw ()
{
	return WARE;
}

void WareInstance::init(Editor_Game_Base & egbase)
{
	Map_Object::init(egbase);
}

void WareInstance::cleanup(Editor_Game_Base & egbase)
{
	// Unlink from our current location, if necessary
	if (upcast(Flag, flag, m_location.get(egbase)))
		flag->remove_item(ref_cast<Game, Editor_Game_Base>(egbase), this);

	delete m_supply;
	m_supply = 0;

	cancel_moving();
	set_location(ref_cast<Game, Editor_Game_Base>(egbase), 0);

	Map_Object::cleanup(egbase);
}

/**
 * Ware accounting
*/
void WareInstance::set_economy(Economy * const e)
{
	if (m_descr_index == Ware_Index::Null() or m_economy == e)
		return;

	if (m_economy)
		m_economy->remove_wares(m_descr_index, 1);

	m_economy = e;
	if (m_supply)
		m_supply->set_economy(e);

	if (m_economy)
		m_economy->add_wares(m_descr_index, 1);
}

/**
 * Change the current location.
 * Once you've assigned a ware to its new location, you usually have to call
 * \ref update() as well.
*/
void WareInstance::set_location
	(Editor_Game_Base & egbase, Map_Object * const location)
{
	Map_Object * const oldlocation = m_location.get(egbase);

	if (oldlocation == location)
		return;

	m_location = location;

	if (location) {
		Economy * eco = 0;

		if (upcast(PlayerImmovable const, playerimmovable, location))
			eco = playerimmovable->get_economy();
		else if (upcast(Worker const, worker, location))
			eco = worker->get_economy();

		if (oldlocation && get_economy()) {
			if (get_economy() != eco)
				throw wexception
					("WareInstance::set_location() implies change of economy");
		} else {
			set_economy(eco);
		}
	}
	else
	{
		set_economy(0);
	}
}

/**
 * Handle delayed updates.
 */
void WareInstance::act(Game & game, uint32_t)
{
	update(game);
}

/**
 * Performs the state updates necessary for the current location:
 * - if it's a building, acknowledge the Request or incorporate into warehouse
 * - if it's a flag and we have no request, start the return to warehouse timer
 * and issue a Supply
 *
 * \note \ref update() may result in the deletion of this object.
 * \note It is important that this function is idempotent, i.e. calling
 *       \ref update() twice in a row should have the same effect as calling
 *       it only once, \em unless the instance is deleted as a side-effect of
 *       \ref update().
 */
void WareInstance::update(Game & game)
{
	Map_Object * const loc = m_location.get(game);

	if (!m_descr) // Upsy, we're not even intialized. Happens on load
		return;

	// Reset our state if we're not on location or outside an economy
	if (!loc || !get_economy()) {
		cancel_moving();
		return;
	}

	// Update whether we have a Supply or not
	if (!m_transfer || !m_transfer->get_request()) {
		if (!m_supply)
			m_supply = new IdleWareSupply(*this);
	} else {
		delete m_supply;
		m_supply = 0;
	}

	// Deal with transfers
	if (m_transfer) {
		upcast(PlayerImmovable, location, loc);
		if (not location)
			return; // wait

		bool success;
		PlayerImmovable * const nextstep =
			m_transfer->get_next_step(location, success);
		m_transfer_nextstep = nextstep;

		if (!nextstep) {
			if (upcast(Flag, flag, location))
				flag->call_carrier(game, *this, 0);

			Transfer * const t = m_transfer;

			m_transfer = 0;

			if (success) {
				t->has_finished();
				return;
			} else {
				t->has_failed();

				cancel_moving();
				update(game);
				return;
			}
		}

		if (upcast(Building, building, location)) {
			if (nextstep != &location->base_flag())
				throw wexception
					("MO(%u): ware: move from building to non-baseflag", serial());

			// There are some situations where we might end up in a warehouse
			// as part of a requested route, and we need to move out of it
			// again, e.g.:
			//  - we were requested just when we were being carried into the
			//    warehouse
			//  - we were carried into a harbour/warehouse to be
			//    shipped across the sea, but a better, land-based route has been
			//    found
			if (upcast(Warehouse, warehouse, building)) {
				warehouse->do_launch_item(game, *this);
				return;
			}

			throw wexception
				("MO(%u): ware: can not move from building %u to %u (not a "
				 "warehouse)",
				 serial(), location->serial(), nextstep->serial());

		} else if (upcast(Flag, flag, location)) {
			flag->call_carrier
				(game,
				 *this,
				 dynamic_cast<Building const *>(nextstep)
				 &&
				 &nextstep->base_flag() != location
				 ?
				 &nextstep->base_flag() : nextstep);
		}
	}
}

/**
 * Set ware state so that it follows the given transfer.
 *
 * \param t the new transfer (non-zero; use \ref cancel_transfer to stop a
 *          transfer).
 */
void WareInstance::set_transfer(Game & game, Transfer & t)
{
	// Reset current transfer
	if (m_transfer) {
		m_transfer->has_failed();
		m_transfer = 0;
	}

	// Set transfer state
	m_transfer = &t;

	delete m_supply;
	m_supply = 0;

	// Schedule an update.
	// Do not update immediately, because update() could try to reference
	// the Transfer object in a way that is not valid yet (note that this
	// function is called in the Transfer constructor before the Transfer
	// is linked to the corresponding Request).
	schedule_act(game, 1, 0);
}

/**
 * The transfer has been cancelled, just stop moving.
*/
void WareInstance::cancel_transfer(Game & game)
{
	m_transfer = 0;

	update(game);
}

/**
 * We are moving when there's a transfer, it's that simple.
*/
bool WareInstance::is_moving() const throw ()
{
	return m_transfer;
}

/**
 * Call this function if movement + potential request need to be cancelled for
 * whatever reason.
*/
void WareInstance::cancel_moving() {
	if (m_transfer) {
		m_transfer->has_failed();
		m_transfer = 0;
	}
}

/**
 * Return the next flag we should be moving to, or the final target if the route
 * has been completed successfully.
*/
PlayerImmovable * WareInstance::get_next_move_step(Game & game)
{
	return
		m_transfer ?
		dynamic_cast<PlayerImmovable *>(m_transfer_nextstep.get(game)) : 0;
}


/*
==============================

Load/save support

==============================
*/

#define WAREINSTANCE_SAVEGAME_VERSION 1

WareInstance::Loader::Loader()
{
}

void WareInstance::Loader::load(FileRead & fr)
{
	Map_Object::Loader::load(fr);

	WareInstance & ware = get<WareInstance>();
	m_location = fr.Unsigned32();
	m_transfer_nextstep = fr.Unsigned32();
	if (fr.Unsigned8()) {
		ware.m_transfer = new Transfer(ref_cast<Game, Editor_Game_Base>(egbase()), ware);
		ware.m_transfer->read(fr, m_transfer);
	}
}

void WareInstance::Loader::load_pointers()
{
	Map_Object::Loader::load_pointers();

	WareInstance & ware = get<WareInstance>();
	ware.set_location(egbase(), &mol().get<Map_Object>(m_location));
	if (m_transfer_nextstep)
		ware.m_transfer_nextstep = &mol().get<Map_Object>(m_transfer_nextstep);
	if (ware.m_transfer)
		ware.m_transfer->read_pointers(mol(), m_transfer);
}

void WareInstance::Loader::load_finish()
{
	Map_Object::Loader::load_finish();

	WareInstance & ware = get<WareInstance>();
	if (!ware.m_transfer || !ware.m_transfer->get_request()) {
		if (!ware.m_supply)
			ware.m_supply = new IdleWareSupply(ware);
	}
}


void WareInstance::save(Editor_Game_Base & egbase, Map_Map_Object_Saver & mos, FileWrite & fw)
{
	fw.Unsigned8(header_WareInstance);
	fw.Unsigned8(WAREINSTANCE_SAVEGAME_VERSION);
	fw.CString(descr().tribe().name());
	fw.CString(descr().name());

	Map_Object::save(egbase, mos, fw);

	fw.Unsigned32(mos.get_object_file_index_or_zero(m_location.get(egbase)));
	fw.Unsigned32(mos.get_object_file_index_or_zero(m_transfer_nextstep.get(egbase)));
	if (m_transfer) {
		fw.Unsigned8(1);
		m_transfer->write(mos, fw);
	} else {
		fw.Unsigned8(0);
	}
}

Map_Object::Loader * WareInstance::load(Editor_Game_Base & egbase, Map_Map_Object_Loader & mol, FileRead & fr)
{
	try {
		uint8_t version = fr.Unsigned8();

		if (version != WAREINSTANCE_SAVEGAME_VERSION)
			throw wexception("unknown/unhandled version %i", version);

		std::string tribename = fr.CString();
		std::string warename = fr.CString();

		egbase.manually_load_tribe(tribename);

		const Tribe_Descr * tribe = egbase.get_tribe(tribename);
		if (!tribe)
			throw wexception("unknown tribe '%s'", tribename.c_str());

		Ware_Index wareindex = tribe->safe_ware_index(warename);
		const Item_Ware_Descr * descr = tribe->get_ware_descr(wareindex);

		std::auto_ptr<Loader> loader(new Loader);
		loader->init(egbase, mol, *new WareInstance(wareindex, descr));
		loader->load(fr);
		return loader.release();
	} catch(const std::exception & e) {
		throw wexception("WareInstance: %s", e.what());
	}
}

}

