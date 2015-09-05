dirname = path.dirname(__file__)

tribes:new_productionsite_type {
   msgctxt = "barbarians_building",
   name = "barbarians_brewery",
   -- TRANSLATORS: This is a building name used in lists of buildings
   descname = pgettext("barbarians_building", "Brewery"),
   directory = dirname,
   icon = dirname .. "menu.png",
   size = "medium",

   enhancement_cost = {
		log = 3,
		granite = 1,
		thatch_reed = 1
	},
	return_on_dismantle_on_enhanced = {
		log = 1,
		granite = 1
	},

   animations = {
		idle = {
			pictures = path.list_directory(dirname, "idle_\\d+.png"),
			hotspot = { 60, 59 },
		},
		working = {
			pictures = path.list_directory(dirname, "idle_\\d+.png"), -- TODO(GunChleoc): No animation yet.
			hotspot = { 60, 59 },
		},
	},

   aihints = {
		prohibited_till = 600,
   },

	working_positions = {
		barbarians_brewer_master = 1,
		barbarians_brewer = 1,
	},

   inputs = {
		water = 8,
		wheat = 8
	},
   outputs = {
		"stout"
   },

	programs = {
		work = {
			-- TRANSLATORS: Completed/Skipped/Did not start brewing stout because ...
			descname = _"brewing stout",
			actions = {
				"sleep=30000",
				"return=skipped unless economy needs stout",
				"consume=water wheat",
				"animate=working 30000",
				"produce=stout"
			}
		},
	},
}
