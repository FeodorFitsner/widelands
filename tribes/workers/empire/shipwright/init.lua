dirname = path.dirname(__file__)

animations = {
   idle = {
      pictures = { dirname .. "idle_\\d+.png" },
      hotspot = { 13, 24 },
      fps = 10
   },
   work = {
      pictures = { dirname .. "work_\\d+.png" },
      sfx = "../../../sound/hammering/hammering",
      hotspot = { 12, 27 },
      fps = 10
   }
}
add_worker_animations(animations, "walk", dirname, "walk", {11, 24}, 10)
add_worker_animations(animations, "walkload", dirname, "walkload", {9, 22}, 10)


tribes:new_worker_type {
   name = "empire_shipyard",
   -- TRANSLATORS: This is a worker name used in lists of workers
   descname = _"Shipyard",
   vision_range = 2,

   buildcost = {
		empire_carrier = 1,
		hammer = 1
	},

	programs = {
		buildship = {
			"walk object-or-coords",
			"plant tribe:shipconstruction unless object",
			"playFX ../../../sound/sawmill/sawmill 230",
			"animation work 500",
			"construct",
			"animation work 5000",
			"return"
		}
	},

	-- TRANSLATORS: Helptext for a worker: Shipyard
   helptext = _"Works at the shipyard and constructs new ships.",
   animations = animations,
}
