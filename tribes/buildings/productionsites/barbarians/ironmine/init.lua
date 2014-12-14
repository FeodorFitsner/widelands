dirname = path.dirname(__file__)

tribes:new_productionsite_type {
   name = "barbarians_ironmine",
   -- TRANSLATORS: This is a building name used in lists of buildings
   descname = _"Iron Mine",
   size = "mine",
   enhancement = "barbarians_ironmine_deep",

   buildcost = {
		log = 4,
		granite = 2
	},
	return_on_dismantle = {
		log = 2,
		granite = 1
	},

	-- #TRANSLATORS: Helptext for a building: Iron Mine
   helptext = "", -- NOCOM(GunChleoc): See what we can shift over from help.lua here

   animations = {
		idle = {
			pictures = { dirname .. "idle_\\d+.png" },
			hotspot = { 21, 36 },
		},
		build = {
			pictures = { dirname .. "build_\\d+.png" },
			hotspot = { 21, 36 },
		},
		working = {
			pictures = { dirname .. "working_\\d+.png" },
			hotspot = { 21, 36 },
		},
		empty = {
			pictures = { dirname .. "empty_\\d+.png" },
			hotspot = { 21, 36 },
		},
	},

   aihints = {
		mines = "iron",
		prohibited_till =900,
		mines_percent = 30
   },

	working_positions = {
		barbarians_miner = 1
	},

   inputs = {
		ration = 6
	},
   outputs = {
		"iron_ore"
   },

	programs = {
		work = {
			-- TRANSLATORS: Completed/Skipped/Did not start mining iron because ...
			descname = _"mining iron",
			actions = {
				"sleep=45000",
				"return=skipped unless economy needs iron_ore",
				"consume=ration",
				"animate=working 20000",
				"mine=iron 2 33 5 17",
				"produce=iron_ore"
			}
		},
	},
	out_of_resource_notification = {
		title = _"Main Iron Vein Exhausted",
		message =
			_"This iron mine’s main vein is exhausted. Expect strongly diminished returns on investment." .. " " ..
			-- TRANSLATORS: "it" is a mine.
			_"You should consider enhancing, dismantling or destroying it.",
		delay_attempts = 0
	},
}
