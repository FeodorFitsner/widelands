dirname = path.dirname(__file__)

animations = {
   idle = {
      pictures = { dirname .. "idle_\\d+.png" },
      hotspot = { 10, 22 }
   }
}
add_worker_animations(animations, "walk", dirname, "walk", {12, 22}, 10)
add_worker_animations(animations, "walkload", dirname, "walkload", {12, 24}, 10)


tribes:new_worker_type {
   name = "atlanteans_smelter",
   -- TRANSLATORS: This is a worker name used in lists of workers
   descname = _"Smelter",

   buildcost = {
		atlanteans_carrier = 1,
		fire_tongs = 1
	},

	-- TRANSLATORS: Helptext for a worker: Smelter
   helptext = _"Smelts iron.",
   animations = animations,
}
