dirname = path.dirname(__file__)

tribes:new_warehouse_type {
   name = "barbarians_port",
   -- TRANSLATORS: This is a building name used in lists of buildings
   descname = _"Port",
   icon = dirname .. "menu.png",
   size = "port",

   buildcost = {
		log = 3,
		blackwood = 3,
		granite = 5,
		grout = 2,
		iron = 2,
		thatch_reed = 4,
		gold = 2
	},
	return_on_dismantle = {
		log = 1,
		blackwood = 2,
		granite = 3,
		grout = 1,
		iron = 1,
		thatch_reed = 1,
		gold = 1
	},

   helptexts = {
		-- TRANSLATORS: Lore helptext for a building
		lore = _"Text needed",
		-- TRANSLATORS: Lore author helptext for a building
		lore_author = _"Source needed",
		-- TRANSLATORS: Purpose helptext for a building
		purpose = _"Text needed",
		-- #TRANSLATORS: Note helptext for a building
		note = "",
		-- #TRANSLATORS: Performance helptext for a building
		performance = ""
   }

   animations = {
		idle = {
			pictures = { dirname .. "idle_\\d+.png" },
			hotspot = { 67, 80 },
			fps = 10
		},
		build = {
			pictures = { dirname .. "build_\\d+.png" },
			hotspot = { 67, 80 }
		}
	},

   conquers = 5,
   heal_per_second = 170,
}
