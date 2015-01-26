dirname = path.dirname(__file__)

tribes:new_militarysite_type {
   name = "atlanteans_guardhouse",
   -- TRANSLATORS: This is a building name used in lists of buildings
   descname = _"Guardhouse",
   size = "small",

   buildcost = {
		log = 1,
		planks = 1,
		granite = 1
	},
	return_on_dismantle = {
		granite = 1
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
			hotspot = { 33, 41 },
		}
	},

	outputs = {
		"atlanteans_soldier",
   },

   max_soldiers = 2,
   heal_per_second = 75,
   conquers = 6,
   prefer_heroes = false,

   aihints = {
		expansion = true,
		mountain_conqueror = true
   },

   messages = {
		occupied = _"Your soldiers have occupied your guardhouse.",
		aggressor = _"Your guardhouse discovered an aggressor.",
		attack = _"Your guardhouse is under attack.",
		defeated_enemy = _"The enemy defeated your soldiers at the guardhouse.",
		defeated_you = _"Your soldiers defeated the enemy at the guardhouse."
   },
}
