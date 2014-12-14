dirname = path.dirname(__file__)

tribes:new_productionsite_type {
   name = "barbarians_granitemine",
   -- TRANSLATORS: This is a building name used in lists of buildings
   descname = _"Granite Mine",
   size = "mine",

   buildcost = {
		log = 4,
		granite = 2
	},
	return_on_dismantle = {
		log = 2,
		granite = 1
	},

	-- TRANSLATORS: Helptext for a building: Granite Mine
   helptext = "", -- NOCOM(GunChleoc): See what we can shift over from help.lua here

   animations = {
		idle = {
			pictures = { dirname .. "idle_\\d+.png" },
			hotspot = { 42, 35 },
		},
		build = {
			pictures = { dirname .. "build_\\d+.png" },
			hotspot = { 42, 35 },
		},
		working = {
			pictures = { dirname .. "working_\\d+.png" },
			hotspot = { 42, 35 },
		},
		empty = {
			pictures = { dirname .. "empty_\\d+.png" },
			hotspot = { 42, 35 },
		},
	},

   aihints = {
		mines = "granite",
		prohibited_till = 900
   },

	working_positions = {
		barbarians_miner = 1
	},

   inputs = {
		ration = 8
	},
   outputs = {
		"granite"
   },

	programs = {
		work = {
			-- TRANSLATORS: Completed/Skipped/Did not start mining granite because ...
			descname = _"mining granite",
			actions = {
				"sleep=20000",
				"return=skipped unless economy needs granite",
				"consume=ration",
				"animate=working 20000",
				"mine=granite 2 100 5 17",
				"produce=granite:2"
			}
		},
	},
	out_of_resource_notification = {
		title = _"Main Granite Vein Exhausted",
		message =
			_"This granite mine’s main vein is exhausted. Expect strongly diminished returns on investment." .. " " ..
			-- TRANSLATORS: "it" is a mine.
			_" This mine can’t be enhanced any further, so you should consider dismantling or destroying it.",
		delay_attempts = 0
	},
}
