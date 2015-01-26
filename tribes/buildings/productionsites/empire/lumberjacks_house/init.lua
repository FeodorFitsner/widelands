dirname = path.dirname(__file__)

tribes:new_productionsite_type {
   name = "empire_lumberjacks_house",
   -- TRANSLATORS: This is a building name used in lists of buildings
   descname = _"Lumberjack’s House",
   size = "small",

   buildcost = {
		log = 2,
		planks = 1
	},
	return_on_dismantle = {
		log = 1,
		planks = 1
	},

   helptexts = {
		-- TRANSLATORS: Lore helptext for a building
		lore = _"Text needed",
		-- TRANSLATORS: Lore author helptext for a building
		lore_author = _"Source needed",
		-- TRANSLATORS: Purpose helptext for a building
		purpose = _"Fells trees in the surrounding area and processes them into logs.",
		-- TRANSLATORS: Note helptext for a building
		note = _"The lumberjack's house needs trees to fell within the working radius.",
		-- TRANSLATORS: Performance helptext for a building
		performance = _"Calculation needed"
   }

   animations = {
		idle = {
			pictures = { dirname .. "idle_\\d+.png" },
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
		message = _"The lumberjack working at this lumberjack’s house can’t find any trees in his working radius. You should consider dismantling or destroying the building or building a forester’s house.",
		delay_attempts = 60
	},
}
