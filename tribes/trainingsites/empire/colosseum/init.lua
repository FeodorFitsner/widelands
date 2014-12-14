dirname = path.dirname(__file__)

tribes:new_militarysite_type {
   name = "empire_colosseum",
   -- TRANSLATORS: This is a building name used in lists of buildings
   descname = _"Colosseum",
   size = "big",
   buildable = false,
   enhanced_building = true,


   enhancement_cost = {
		planks = 2,
		granite = 4,
		marble = 4,
		cloth = 2,
		gold = 4,
		marble_column = 4
	},
	return_on_dismantle_on_enhanced = {
		planks = 1,
		granite = 2,
		marble = 2,
		gold = 2,
		marble_column = 2
	},

	-- TRANSLATORS: Helptext for a building: Colosseum
   helptext = "", -- NOCOM(GunChleoc): See what we can shift over from help.lua here

   animations = {
		idle = {
			pictures = { dirname .. "idle_\\d+.png" },
			hotspot = { 81, 106 }
		}
	},

	working_positions = {
		empire_trainer = 1
	},

	inputs = {
		bread_empire = 10,
		fish = 6,
		meat = 6
	},

	["soldier evade"] = {
		min_level = 0,
		max_level = 1
	},

	programs = {
		sleep = {
			-- TRANSLATORS: Completed/Skipped/Did not start sleeping because ...
			descname = _"sleeping",
			actions = {
				"sleep=5000",
				"check_soldier=soldier attack 9", -- dummy check to get sleep rated as skipped - else it will change statistics
			}
		},
		upgrade_soldier_evade_0 = {
			-- TRANSLATORS: Completed/Skipped/Did not start upgrading ... because ...
			descname = _"upgrading soldier evade from level 0 to level 1",
			actions = {
				"check_soldier=soldier evade 0", -- Fails when aren't any soldier of level 0 evade
				"sleep=30000",
				"check_soldier=soldier evade 0", -- Because the soldier can be expelled by the player
				"consume=bread_empire:2 fish,meat",
				"train=soldier evade 0 1"
			}
		},
		upgrade_soldier_evade_1 = {
			-- TRANSLATORS: Completed/Skipped/Did not start upgrading ... because ...
			descname = _"upgrading soldier evade from level 1 to level 2",
			actions = {
				"check_soldier=soldier evade 1", -- Fails when aren't any soldier of level 1 evade
				"sleep=30000",
				"check_soldier=soldier evade 1", -- Because the soldier can be expelled by the player
				"consume=bread_empire:2 fish,meat:2",
				"train=soldier evade 1 2"
			}
		},
	}

   soldier_capacity = 8,
   trainer_patience = 9
}
