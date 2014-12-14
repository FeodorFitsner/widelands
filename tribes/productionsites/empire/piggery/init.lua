dirname = path.dirname(__file__)

tribes:new_productionsite_type {
   name = "empire_piggery",
   -- TRANSLATORS: This is a building name used in lists of buildings
   descname = _"Piggery",
   size = "big",

   buildcost = {
		log = 2,
		granite = 2,
		marble_column = 2
	},
	return_on_dismantle = {
		log = 1,
		granite = 1,
		marble = 1
	},

	-- TRANSLATORS: Helptext for a building: Piggery
   helptext = "", -- NOCOM(GunChleoc): See what we can shift over from help.lua here

   animations = {
		idle = {
			pictures = { dirname .. "idle_\\d+.png" },
			hotspot = { 82, 74 },
		},
		working = {
			pictures = { dirname .. "idle_\\d+.png" }, -- TODO(GunChleoc): No animation yet.
			hotspot = { 82, 74 },
		},
	},

	working_positions = {
		empire_pigbreeder = 1
	},

   inputs = {
		wheat = 7,
		water = 7
	},
   outputs = {
		"meat"
   },

	programs = {
		work = {
			-- TRANSLATORS: Completed/Skipped/Did not start raising pigs because ...
			descname = _"raising pigs",
			actions = {
				"sleep=25000",
				"return=skipped unless economy needs meat",
				"consume=water wheat",
				"playFX=../../../sound/farm/farm_animal 180",
				"animate=working 30000",
				"produce=meat"
			}
		},
	},
}
