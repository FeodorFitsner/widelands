dirname = path.dirname(__file__)

tribes:new_productionsite_type {
   name = "empire_coalmine",
   -- TRANSLATORS: This is a building name used in lists of buildings
   descname = _"Coal Mine",
   size = "mine",
   enhancement = "empire_coalmine_deep",

   buildcost = {
		log = 4,
		planks = 2
	},
	return_on_dismantle = {
		log = 2,
		planks = 1
	},

	-- TRANSLATORS: Helptext for a building: Coal Mine
   helptext = "", -- NOCOM(GunChleoc): See what we can shift over from help.lua here

   animations = {
		idle = {
			pictures = { dirname .. "idle_\\d+.png" },
			hotspot = { 49, 49 },
		},
		working = {
			pictures = { dirname .. "working_\\d+.png" },
			hotspot = { 49, 49 },
			fps = 10
		},
		empty = {
			pictures = { dirname .. "empty_\\d+.png" },
			hotspot = { 49, 49 },
		},
	},

   aihints = {
		mines = "coal",
		mines_percent = 50,
		prohibited_till = 1200
   },

	working_positions = {
		empire_miner = 1
	},

   inputs = {
		ration = 6,
		beer = 6
	},
   outputs = {
		"coal"
   },

	programs = {
		work = {
			-- TRANSLATORS: Completed/Skipped/Did not start mining coal because ...
			descname = _"mining coal",
			actions = {
				"sleep=45000",
				"return=skipped unless economy needs coal",
				"consume=beer ration",
				"animate=working 20000",
				"mine=coal 2 50 5 17",
				"produce=coal:2",
				"animate=working 20000",
				"mine=coal 2 50 5 17",
				"produce=coal"
			}
		},
	},
	out_of_resource_notification = {
		title = _"Main Coal Vein Exhausted",
		message =
			_"This coal mine’s main vein is exhausted. Expect strongly diminished returns on investment." .. " " ..
			-- TRANSLATORS: "it" is a mine.
			_"You should consider enhancing, dismantling or destroying it.",
		delay_attempts = 0
	},
}
