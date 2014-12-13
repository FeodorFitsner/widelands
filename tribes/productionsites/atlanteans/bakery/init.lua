dirname = path.dirname(__file__)

tribes:new_productionsite_type {
   name = "atlanteans_bakery",
   -- TRANSLATORS: This is a building name used in lists of buildings
   descname = _"Bakery",
   size = "medium",

   buildcost = {
		log = 2,
		planks = 2,
		granite = 3
	},
	return_on_dismantle = {
		planks = 1,
		granite = 2
	},

	-- TRANSLATORS: Helptext for a building: Bakery
   helptext = "", -- NOCOM(GunChleoc): See what we can shift over from help.lua here

   animations = {
		idle = {
			pictures = { dirname .. "idle_\\d+.png" },
			hotspot = { 52, 63 },
		},
		working = {
			pictures = { dirname .. "working_\\d+.png" },
			hotspot = { 52, 75 },
			fps = 20
		}
	},

   aihints = {
		forced_after=1200,
		prohibited_till=900
   },

	working_positions = {
		atlanteans_baker = 1
	},

   inputs = {
		blackroot_flour = 4,
		cornmeal = 4,
		water = 8
	},
   outputs = {
		"bread_atlanteans"
   },

	programs = {
		work = {
			-- TRANSLATORS: Completed/Skipped/Did not start baking bread because ...
			descname = _"baking bread",
			actions = {
				"sleep=35000",
				"return=skipped unless economy needs bread_atlanteans",
				"consume=water:2 blackroot_flour cornmeal",
				"animate=working 30000",
				"produce=bread_atlanteans:2"
			}
		},
	},
}
