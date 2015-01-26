dirname = path.dirname(__file__)

tribes:new_productionsite_type {
   name = "empire_weaving_mill",
   -- TRANSLATORS: This is a building name used in lists of buildings
   descname = _"Weaving Mill",
   size = "big",

   buildcost = {
		log = 3,
		granite = 4,
		marble = 1
	},
	return_on_dismantle = {
		log = 1,
		granite = 3
	},

   helptexts = {
		-- TRANSLATORS: Lore helptext for a building
		lore = _"Text needed",
		-- TRANSLATORS: Lore author helptext for a building
		lore_author = _"Source needed",
		-- TRANSLATORS: Purpose helptext for a building
		purpose = _"Weaves cloth out of wool.",
		-- #TRANSLATORS: Note helptext for a building
		note = "",
		-- TRANSLATORS: Performance helptext for a building
		performance = _"Calculation needed"
   }

   animations = {
		idle = {
			pictures = { dirname .. "idle_\\d+.png" },
			hotspot = { 65, 62 },
		},
		build = {
			pictures = { dirname .. "build_\\d+.png" },
			hotspot = { 65, 62 },
		},
		unoccupied = {
			pictures = { dirname .. "unoccupied_\\d+.png" },
			hotspot = { 65, 62 },
		},
		working = {
			pictures = { dirname .. "working_\\d+.png" },
			hotspot = { 65, 62 },
			fps = 5
		},
	},

   aihints = {
		prohibited_till = 120
   },

	working_positions = {
		empire_weaver = 1
	},

   inputs = {
		wool = 8
	},
   outputs = {
		"cloth"
   },

	programs = {
		work = {
			-- TRANSLATORS: Completed/Skipped/Did not start weaving because ...
			descname = _"weaving",
			actions = {
				"sleep=25000",
				"return=skipped unless economy needs cloth",
				"consume=wool",
				"animate=working 15000", -- Unsure of balancing CW
				"produce=cloth"
			}
		},
	},
}
