dirname = path.dirname(__file__)

tribes:new_productionsite_type {
   name = "atlanteans_blackroot_farm",
   -- TRANSLATORS: This is a building name used in lists of buildings
   descname = _"Blackroot Farm",
   icon = dirname .. "menu.png",
   size = "big",

   buildcost = {
		planks = 3,
		granite = 2,
		log = 4
	},
	return_on_dismantle = {
		planks = 1,
		granite = 2,
		log = 1
	},

   helptexts = {
		-- TRANSLATORS: Lore helptext for a building
		lore = _"Text needed",
		-- TRANSLATORS: Lore author helptext for a building
		lore_author = _"Source needed",
		-- TRANSLATORS: Purpose helptext for a building
		purpose = _"Sow and harvest blackroot.",
		-- #TRANSLATORS: Note helptext for a building
		note = "",
		-- TRANSLATORS: Performance helptext for a building
		performance = _"Calculation needed"
   }

   animations = {
		idle = {
			pictures = { dirname .. "idle_\\d+.png" },
			hotspot = { 74, 60 },
		},
	},

   aihints = {
		prohibited_till = 600,
		space_consumer = true
   },

	working_positions = {
		atlanteans_blackroot_farmer = 1
	},

   outputs = {
		"blackroot"
   },

	programs = {
		work = {
			-- TRANSLATORS: Completed/Skipped/Did not start working because ...
			descname = _"working",
			actions = {
				"call=plant_blackroot",
				"call=harvest_blackroot",
				"return=skipped"
			}
		},
		plant_blackroot = {
			-- TRANSLATORS: Completed/Skipped/Did not start planting blackroot because ...
			descname = _"planting blackroot",
			actions = {
				"sleep=20000",
				"worker=plant"
			}
		},
		harvest_blackroot = {
			-- TRANSLATORS: Completed/Skipped/Did not start harvesting blackroot because ...
			descname = _"harvesting blackroot",
			actions = {
				"sleep=5000",
				"worker=harvest"
			}
		},
	},
	out_of_resource_notification = {
		title = _"Out of Fields",
		message = _"The farmer working at this blackroot farm has no cleared soil to plant his seeds.",
		delay_attempts = 10
	},
}
