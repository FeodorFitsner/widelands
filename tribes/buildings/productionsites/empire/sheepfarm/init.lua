dirname = path.dirname(__file__)

tribes:new_productionsite_type {
   name = "empire_sheepfarm",
   -- TRANSLATORS: This is a building name used in lists of buildings
   descname = _"Sheep Farm",
   size = "big",

   buildcost = {
		log = 2,
		granite = 2,
		planks = 2
	},
	return_on_dismantle = {
		log = 1,
		granite = 2
	},

	-- TRANSLATORS: Helptext for a building: Sheep Farm
   helptext = "", -- NOCOM(GunChleoc): See what we can shift over from help.lua here

   animations = {
		idle = {
			pictures = { dirname .. "idle_\\d+.png" },
			hotspot = { 73, 60 },
		},
		working = {
			pictures = { dirname .. "idle_\\d+.png" }, -- TODO(GunChleoc): No animation yet.
			hotspot = { 73, 60 },
		},
	},

   aihints = {
		prohibited_till = 600
   },

	working_positions = {
		empire_shepherd = 1
	},

   inputs = {
		wheat = 7,
		water = 7
	},
   outputs = {
		"wool"
   },

	programs = {
		work = {
			-- TRANSLATORS: Completed/Skipped/Did not start breeding sheep because ...
			descname = _"breeding sheep",
			actions = {
				"sleep=25000",
				"return=skipped unless economy needs wool",
				"consume=water wheat",
				"playFX=../../../sound/farm/sheep 192",
				"animate=working 30000",
				"produce=wool"
			}
		},
	},
}
