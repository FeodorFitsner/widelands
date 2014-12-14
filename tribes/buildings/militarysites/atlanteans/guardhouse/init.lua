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

	-- TRANSLATORS: Helptext for a militarysite: Guardhouse
   helptext = "", -- NOCOM(GunChleoc): See what we can shift over from help.lua here

   animations = {
		idle = {
			pictures = { dirname .. "idle_\\d+.png" },
			hotspot = { 33, 41 },
		}
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
