dirname = path.dirname(__file__)

tribes:new_productionsite_type {
   name = "barbarians_ironmine_deeper",
   -- TRANSLATORS: This is a building name used in lists of buildings
   descname = _"Deeper Iron Mine",
   size = "mine",
   buildable = false,
   enhanced_building = true,

   enhancement_cost = {
		log = 4,
		granite = 2
	},
	return_on_dismantle_on_enhanced = {
		log = 2,
		granite = 1
	},

	-- #TRANSLATORS: Helptext for a building: Deeper Iron Mine
   helptext = "", -- NOCOM(GunChleoc): See what we can shift over from help.lua here

   animations = {
		idle = {
			pictures = { dirname .. "idle_\\d+.png" },
			hotspot = { 60, 37 },
		},
		build = {
			pictures = { dirname .. "build_\\d+.png" },
			hotspot = { 60, 37 },
		},
		working = {
			pictures = { dirname .. "working_\\d+.png" },
			hotspot = { 60, 37 },
		},
		empty = {
			pictures = { dirname .. "empty_\\d+.png" },
			hotspot = { 60, 37 },
		},
	},

   aihints = {
		mines = "iron",
   },

	working_positions = {
		barbarians_miner = 1,
		barbarians_miner_chief = 1,
		barbarians_miner_master = 1,
	},

   inputs = {
		meal = 6
	},
   outputs = {
		"iron_ore"
   },

	programs = {
		work = {
			-- TRANSLATORS: Completed/Skipped/Did not start mining iron because ...
			descname = _"mining iron",
			actions = {
				"sleep=40000",
				"return=skipped unless economy needs iron_ore",
				"consume=meal",
				"animate=working 16000",
				"mine=iron 2 100 10 2",
				"produce=iron_ore",
				"animate=working 16000",
				"mine=iron 2 100 10 2",
				"produce=iron_ore:2",
				"animate=working 16000",
				"mine=iron 2 100 10 2",
				"produce=iron_ore:2"
			}
		},
	},
	out_of_resource_notification = {
		title = _"Main Iron Vein Exhausted",
		message =
			_"This iron mine’s main vein is exhausted. Expect strongly diminished returns on investment." .. " " ..
			-- TRANSLATORS: "it" is a mine.
			_"This mine can’t be enhanced any further, so you should consider dismantling or destroying it.",
		delay_attempts = 0
	},
}
