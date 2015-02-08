dirname = path.dirname(__file__)

animations = {
   idle = {
      pictures = { dirname .. "idle_\\d+.png" },
      hotspot = { 16, 31 },
      fps = 5
   },
   atk_ok_e = {
      pictures = { dirname .. "atk_ok_e_\\d+.png" },
      hotspot = { 30, 36 },
      fps = 10
   },
   atk_fail_e = {
      pictures = { dirname .. "atk_fail_e\\d+.png" },
      hotspot = { 30, 36 },
      fps = 10
   },
   eva_ok_e = {
      pictures = { dirname .. "eva_ok_e_\\d+.png" },
      hotspot = { 18, 34 },
      fps = 20
   },
   eva_fail_e = {
      pictures = { dirname .. "eva_fail_e_\\d+.png" },
      hotspot = { 30, 36 },
      fps = 10
   },
   atk_ok_w = {
      pictures = { dirname .. "atk_ok_w_\\d+.png" },
      hotspot = { 30, 36 },
      fps = 10
   },
   atk_fail_w = {
      pictures = { dirname .. "atk_fail_w_\\d+.png" },
      hotspot = { 30, 36 },
      fps = 10
   },
   eva_ok_w = {
      pictures = { dirname .. "eva_ok_w_\\d+.png" },
      hotspot = { 18, 34 },
      fps = 20
   },
   eva_fail_w = {
      pictures = { dirname .. "eva_fail_w_\\d+.png" },
      hotspot = { 30, 36 },
      fps = 10
   },
   die_w = {
      pictures = { dirname .. "die_\\d+.png" },
      hotspot = { 16, 31 },
      fps = 20
   },
   die_e = {
      pictures = { dirname .. "die_\\d+.png" },
      hotspot = { 16, 31 },
      fps = 20
   },
}
add_worker_animations(animations, "walk", dirname, "walk", {16, 31}, 10)


tribes:new_soldier_type {
   name = "barbarians_soldier",
   -- TRANSLATORS: This is a worker name used in lists of workers
   descname = _"Soldier",
   vision_range = 2,

   buildcost = {
		barbarians_carrier = 1,
		ax = 1
	},

	-- TRANSLATORS: Helptext for a worker: Soldier
   helptext = _"Defend and Conquer!",
   animations = animations,

	max_hp_level = 3,
	max_attack_level = 5,
	max_defense_level = 0,
	max_evade_level = 2,

	-- initial values and per level increasements
	hp = 13000,
	hp_incr_per_level = 2800,
	attack = {
		minimum = 1200,
		maximum = 1600
	},
	attack_incr_per_level = 700,
	defense = 3,
	defense_incr_per_level = 4,
	evade = 25,
	evade_incr_per_level = 15,

	hp_level_0_pic = dirname .. "hp_level0.png",
	hp_level_1_pic = dirname .. "hp_level1.png",
	hp_level_2_pic = dirname .. "hp_level2.png",
	hp_level_3_pic = dirname .. "hp_level3.png",
	evade_level_0_pic = dirname .. "evade_level0.png",
	evade_level_1_pic = dirname .. "evade_level1.png",
	evade_level_2_pic = dirname .. "evade_level2.png",
	attack_level_0_pic = dirname .. "attack_level0.png",
	attack_level_1_pic = dirname .. "attack_level1.png",
	attack_level_2_pic = dirname .. "attack_level2.png",
	attack_level_3_pic = dirname .. "attack_level3.png",
	attack_level_4_pic = dirname .. "attack_level4.png",
	attack_level_5_pic = dirname .. "attack_level5.png",
	defense_level_0_pic = dirname .. "defense_level0.png",

	-- Random animations for battle
	attack_success_w = {
		"atk_ok_w",
	},
	attack_success_e = {
		"atk_ok_e",
	},
	attack_failure_w = {
		"atk_fail_w",
	},
	attack_failure_e = {
		"atk_fail_e",
	},
	evade_success_w = {
		"eva_ok_w",
	},
	evade_success_e = {
		"eva_ok_e",
	}
	evade_failure_w = {
		"eva_fail_w",
	}
	evade_failure_e = {
		"eva_fail_e",
	}
	die_w = {
		"die_w",
	}
	die_e = {
		"die_e",
	}
}
