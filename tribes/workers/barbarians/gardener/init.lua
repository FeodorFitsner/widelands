dirname = path.dirname(__file__)

animations = {
   idle = {
      pictures = { dirname .. "idle_\\d+.png" },
      hotspot = { -4, 11 }
   },
   planting = {
      pictures = { dirname .. "plantreed_\\d+.png" },
      hotspot = { 10, 21 },
      fps = 10
   },
   harvesting = {
      pictures = { dirname .. "harvest_\\d+.png" },
      hotspot = { 10, 22 },
      fps = 5
   }
}
add_worker_animations(animations, "walk", dirname, "walk", {8, 23}, 10)
add_worker_animations(animations, "walkload", dirname, "walkload", {7, 23}, 10)


tribes:new_worker_type {
   name = "barbarians_gardener",
   -- TRANSLATORS: This is a worker name used in lists of workers
   descname = _"Gardener",
   vision_range = 2,

   buildcost = {
		barbarians_carrier = 1,
		shovel = 1
	},

	programs = {
		plantreed = {
			"findspace size:any radius:1",
			"walk coords",
			"animation planting 1500",
			"plant tribe:reed_tiny",
			"animation planting 1500",
			"return"
		},
		harvestreed = {
			"findobject attrib:ripe_reed radius:1",
			"walk object",
			"animation harvesting 12000",
			"object harvest",
			"animation harvesting 1",
			"createware thatch_reed",
			"return"
		},
	},

	-- TRANSLATORS: Helptext for a worker: Gardener
   helptext = _"Grows thatch reed.",
   animations = animations,
}
