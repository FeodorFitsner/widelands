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

#ifndef __S__WIDELANDS_ATTACK_CTRL_H
#define __S__WIDELANDS_ATTACK_CTRL_H

#include "instances.h"

#include <set>
#include <vector>
#include <types.h>
#include "immovable.h"

class Game;
class Flag;
class MilitarySite;
class Soldier;
class Coords;
class Widelands_Map_Attack_Controller_Data_Packet;


void getCloseMilitarySites
(const Game &,
 const Flag &,
 const Player_Number player,
 std::set<MilitarySite *> & militarySites);
uint getMaxAttackSoldiers(const Game &, const Flag &, const Player_Number);

class AttackController : public BaseImmovable {
   friend class Widelands_Map_Attack_Controller_Data_Packet;

   public:
      AttackController(Game* game, Flag* flag, int attacker, int defender);
      AttackController(Game* game);
      ~AttackController();
      void launchAttack(uint nrAttackers);

      //Methods inherited by BaseImmovable
      virtual int  get_type    () const throw () {return ATTACKCONTROLLER;}
      virtual int  get_size    () const throw () {return SMALL;}
      virtual bool get_passable() const throw () {return false;}
      virtual void draw (const Editor_Game_Base &, RenderTarget &, const FCoords, const Point){}
      virtual void act (Game*, uint);
      virtual void cleanup (Editor_Game_Base*);
      //end inherited

      void moveToReached(Soldier* soldier);
      void soldierDied(Soldier* soldier);
      void soldierWon(Soldier* soldier);

      inline int getAttackingPlayer() { return attackingPlayer; };
      inline int getDefendingPlayer() { return defendingPlayer; };
      inline Flag* getFlag() { return flag; };
      inline Game* getGame() { return game; };
   private:
      struct BattleSoldier {
         Soldier* soldier;
         MilitarySite* origin;
         Coords* battleGround;
         bool attacker;
         bool arrived;
         bool fighting;
      };

      bool battleGroundOccupied(Coords* coords);
      void calcBattleGround(BattleSoldier*, int);

      bool launchAllSoldiers(bool attackers, int nrLaunch);
      void launchSoldiersOfMilitarySite(MilitarySite* militarySite, uint nrLaunch, bool attackers);
      bool moveToBattle(Soldier* soldier, MilitarySite* militarySite, bool attackers);

      bool startBattle(Soldier*, bool);
      void removeSoldier(Soldier*);
      uint getBattleSoldierIndex(Soldier*);
      bool opponentsLeft(Soldier* soldier);

      std::vector<BattleSoldier> involvedSoldiers;
      std::set<Object_Ptr> involvedMilitarySites;

      int attackingPlayer;
      int defendingPlayer;
      uint totallyLaunched;
      bool attackedMsEmpty;
      Flag* flag;
      Game* game;

};

#endif
