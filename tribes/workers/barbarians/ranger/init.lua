dirname = path.dirname(__file__)

animations = {
   idle = {
      pictures = { dirname .. "idle_\\d+.png" },
      hotspot = { 11, 23 }
   },
   dig = {
      pictures = { dirname .. "dig_\\d+.png" },
      hotspot = { 12, 24 },
      fps = 5
   },
   crop = {
      pictures = { dirname .. "plant_\\d+.png" },
      hotspot = { 18, 24 },
      fps = 10
   },
   water = {
      pictures = { dirname .. "water_\\d+.png" },
      hotspot = { 19, 25 },
      fps = 5
   }
}
}
add_worker_animations(animations, "walk", dirname, "walk", {11, 23}, 10)
add_worker_animations(animations, "walkload", dirname, "walkload", {11, 23}, 10)


tribes:new_worker_type {
   name = "barbarians_ranger",
   -- TRANSLATORS: This is a worker name used in lists of workers
   descname = _"Ranger",

   buildcost = {
		barbarians_carrier = 1,
		shovel = 1
	},

	programs = {
		plant = {
			"findspace size:any radius:5 avoid:seed",
			"walk coords",
			"animation dig 2000",
			"animation crop 1000",
			"plant attrib:tree_sapling",
			"animation water 2000",
			"return"
		}
	},

	-- TRANSLATORS: Helptext for a worker: Ranger
   helptext = _"Plants trees.",
   animations = animations,
}
