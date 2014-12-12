dirname = path.dirname(__file__)

animations = {
   idle = {
      pictures = { dirname .. "idle_\\d+.png" },
      hotspot = { 4, 23 }
   }
}
add_worker_animations(animations, "walk", dirname, "walk", {18, 25}, 10)
add_worker_animations(animations, "walkload", dirname, "walkload", {8, 22}, 10)


tribes:new_worker_type {
   name = "empire_smelter",
   -- TRANSLATORS: This is a worker name used in lists of workers
   descname = _"Smelter",

   buildcost = {
		empire_carrier = 1,
		fire_tongs = 1
	},

	-- TRANSLATORS: Helptext for a worker: Smelter
   helptext = _"Smelts ores into metal.",
   animations = animations,
}
