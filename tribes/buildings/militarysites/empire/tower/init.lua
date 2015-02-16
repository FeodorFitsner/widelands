dirname = path.dirname(__file__)

tribes:new_militarysite_type {
   name = "empire_tower",
   -- TRANSLATORS: This is a building name used in lists of buildings
   descname = _"Tower",
   icon = dirname .. "menu.png",
   size = "medium",
   vision_range = 19,

   buildcost = {
		log = 2,
		planks = 3,
		granite = 4,
		marble_column = 2
	},
	return_on_dismantle = {
		planks = 1,
		granite = 2,
		marble_column = 1
	},

   helptexts = {
		-- TRANSLATORS: Lore helptext for a building
		lore = _"Text needed",
		-- TRANSLATORS: Lore author helptext for a building
		lore_author = _"Source needed",
		-- TRANSLATORS: Purpose helptext for a building
		purpose = _"Garrisons soldiers to expand your territory.",
		-- TRANSLATORS: Note helptext for a building
		note = _"If you’re low on soldiers to occupy new military sites, use the downward arrow button to decrease the capacity. You can also click on a soldier to send him away.",
		-- #TRANSLATORS: Performance helptext for a building
		performance = ""
   }

   animations = {
		idle = {
			pictures = { dirname .. "idle_\\d+.png" },
			hotspot = { 53, 81 }
		},
		build = {
			pictures = { dirname .. "build_\\d+.png" },
			hotspot = { 53, 81 }
		}
	},

	outputs = {
		"empire_soldier",
   },

	max_soldiers = 5,
   heal_per_second = 150,
   conquers = 9,
   prefer_heroes = true,

   aihints = {
		mountain_conqueror = true,
		prohibited_till = 300
   },

   messages = {
		occupied = _"Your soldiers have occupied your tower.",
		aggressor = _"Your tower discovered an aggressor.",
		attack = _"Your tower is under attack.",
		defeated_enemy = _"The enemy defeated your soldiers at the tower.",
		defeated_you = _"Your soldiers defeated the enemy at the tower."
   },
}
