dirname = path.dirname(__file__)

tribes:new_warehouse_type {
   name = "empire_port",
   -- TRANSLATORS: This is a building name used in lists of buildings
   descname = _"Port",
   icon = dirname .. "menu.png",
   size = "port",

   buildcost = {
		log = 3,
		planks = 4,
		granite = 4,
		marble = 2,
		marble_column = 1,
		cloth = 3,
		gold = 2
	},
	return_on_dismantle = {
		log = 1,
		planks = 1,
		granite = 2,
		marble = 2,
		cloth = 1,
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
			hotspot = { 74, 96 },
			fps = 10
		},
		build = {
			pictures = { dirname .. "build_\\d+.png" },
			hotspot = { 74, 96 }
		}
	},

   conquers = 5,
   heal_per_second = 170,
}
