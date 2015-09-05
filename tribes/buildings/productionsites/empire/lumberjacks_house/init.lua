dirname = path.dirname(__file__)

tribes:new_productionsite_type {
   msgctxt = "empire_building",
   name = "empire_lumberjacks_house",
   -- TRANSLATORS: This is a building name used in lists of buildings
   descname = pgettext("empire_building", "Lumberjack’s House"),
   directory = dirname,
   icon = dirname .. "menu.png",
   size = "small",

   buildcost = {
		log = 2,
		planks = 1
	},
	return_on_dismantle = {
		log = 1,
		planks = 1
	},

   animations = {
		idle = {
			pictures = path.list_directory(dirname, "idle_\\d+.png"),
			hotspot = { 40, 59 },
		},
	},

   aihints = {
		forced_after = 0,
		logproducer = true
   },

	working_positions = {
		empire_lumberjack = 1
	},

   outputs = {
		"log"
   },

	programs = {
		work = {
			-- TRANSLATORS: Completed/Skipped/Did not start felling trees because ...
			descname = _"felling trees",
			actions = {
				"sleep=30000", -- Barbarian lumberjack sleeps 25000
				"worker=chop"
			}
		},
	},
	out_of_resource_notification = {
		title = _"Out of Trees",
		message = _"The lumberjack working at this lumberjack’s house can’t find any trees in his work area. You should consider dismantling or destroying the building or building a forester’s house.",
		productivity_threshold = 66
	},
}
