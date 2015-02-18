dirname = path.dirname(__file__)

tribes:new_militarysite_type {
   name = "atlanteans_guardhall",
   -- TRANSLATORS: This is a building name used in lists of buildings
   descname = _"Guardhall",
   icon = dirname .. "menu.png",
   size = "medium",

   buildcost = {
		log = 2,
		planks = 3,
		granite = 4,
		diamond = 1
	},
	return_on_dismantle = {
		log = 1,
		planks = 1,
		granite = 3
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
   },

   aihints = {
		fighting = true
   },

   animations = {
		idle = {
			pictures = path.list_directory(dirname, "idle_\\d+.png"),
			hotspot = { 58, 72 },
		}
	},

	outputs = {
		"atlanteans_soldier",
   },

	max_soldiers = 7,
   heal_per_second = 140,
   conquers = 7,
   prefer_heroes = true,

   messages = {
		occupied = _"Your soldiers have occupied your guardhall.",
		aggressor = _"Your guardhall discovered an aggressor.",
		attack = _"Your guardhall is under attack.",
		defeated_enemy = _"The enemy defeated your soldiers at the guardhall.",
		defeated_you = _"Your soldiers defeated the enemy at the guardhall."
   },
}
