dirname = path.dirname(__file__)

tribes:new_trainingsite_type {
   name = "atlanteans_dungeon",
   -- TRANSLATORS: This is a building name used in lists of buildings
   descname = _"Dungeon",
   size = "medium",

   buildcost = {
		planks = 2,
		granite = 4,
		diamond = 2,
		quartz = 2,
		spidercloth = 2,
		gold = 2,
		log = 4
	},
	return_on_dismantle = {
		planks = 1,
		granite = 3,
		diamond = 1,
		quartz = 1,
		gold = 1,
		log = 1
	},

	-- #TRANSLATORS: Helptext for a building: Dungeon
   helptext = "", -- NOCOM(GunChleoc): See what we can shift over from help.lua here

   animations = {
		idle = {
			pictures = { dirname .. "idle_\\d+.png" },
			hotspot = { 47, 47 },
		}
	},

	working_positions = {
		atlanteans_trainer = 1
	},

	inputs = {
		bread_atlanteans = 10,
		smoked_fish = 6,
		smoked_meat = 6,
		trident_long = 4,
		trident_steel = 4,
		trident_double = 4,
		trident_heavy_double = 4
	},

	["soldier attack"] = {
		min_level = 0,
		max_level = 3
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
		upgrade_soldier_attack_0 = {
			-- TRANSLATORS: Completed/Skipped/Did not start upgrading ... because ...
			descname = _"upgrading soldier attack from level 0 to level 1",
			actions = {
				"check_soldier=soldier attack 0",
				"sleep=30000",
				"check_soldier=soldier attack 0",
				"consume=bread_atlanteans smoked_fish,smoked_meat trident_long",
				"train=soldier attack 0 1"
			}
		},
		upgrade_soldier_attack_1 = {
			-- TRANSLATORS: Completed/Skipped/Did not start upgrading ... because ...
			descname = _"upgrading soldier attack from level 1 to level 2",
			actions = {
				"check_soldier=soldier attack 1",
				"sleep=30000",
				"check_soldier=soldier attack 1",
				"consume=bread_atlanteans smoked_fish,smoked_meat trident_steel",
				"train=soldier attack 1 2"
			}
		},
		upgrade_soldier_attack_2 = {
			-- TRANSLATORS: Completed/Skipped/Did not start upgrading ... because ...
			descname = _"upgrading soldier attack from level 2 to level 3",
			actions = {
				"check_soldier=soldier attack 2",
				"sleep=30000",
				"check_soldier=soldier attack 2",
				"consume=bread_atlanteans smoked_fish,smoked_meat trident_double",
				"train=soldier attack 2 3"
			}
		},
		upgrade_soldier_attack_3 = {
			-- TRANSLATORS: Completed/Skipped/Did not start upgrading ... because ...
			descname = _"upgrading soldier attack from level 3 to level 4",
			actions = {
				"check_soldier=soldier attack 3",
				"sleep=30000",
				"check_soldier=soldier attack 3",
				"consume=bread_atlanteans smoked_fish,smoked_meat trident_heavy_double",
				"train=soldier attack 3 4"
			}
		},
	}

   soldier_capacity = 8,
   trainer_patience = 16
}
