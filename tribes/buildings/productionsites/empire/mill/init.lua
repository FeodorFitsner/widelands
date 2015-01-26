dirname = path.dirname(__file__)

tribes:new_productionsite_type {
   name = "empire_mill",
   -- TRANSLATORS: This is a building name used in lists of buildings
   descname = _"Mill",
   size = "medium",

   buildcost = {
		log = 3,
		granite = 3,
		marble = 1
	},
	return_on_dismantle = {
		log = 1,
		granite = 2,
		marble = 1
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
		-- TRANSLATORS: Performance helptext for a building
		performance = _"Calculation needed"
   }

   animations = {
		idle = {
			pictures = { dirname .. "idle_\\d+.png" },
			hotspot = { 41, 87 },
		},
		working = {
			pictures = { dirname .. "working_\\d+.png" }, -- Let's see the mill turn!
			hotspot = { 41, 87 },
			fps = 25
		},
	},

   aihints = {
		prohibited_till = 600
   },

	working_positions = {
		empire_miller = 1
	},

   inputs = {
		wheat = 6
	},
   outputs = {
		"flour"
   },

	programs = {
		work = {
			-- TRANSLATORS: Completed/Skipped/Did not start grinding wheat because ...
			descname = _"grinding wheat",
			actions = {
				"sleep=5000",
				"return=skipped unless economy needs flour",
				"consume=wheat",
				"playFX=../../../sound/mill/mill_turning 240",
				"animate=working 10000",
				"produce=flour"
			}
		},
	},
}
