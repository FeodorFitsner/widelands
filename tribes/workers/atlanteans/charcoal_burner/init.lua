dirname = path.dirname(__file__)

animations = {
   idle = {
      pictures = { dirname .. "waiting_\\d+.png" },
      hotspot = { 4, 22 }
   }
}
add_worker_animations(animations, "walk", dirname, "walk", {8, 22}, 10)
add_worker_animations(animations, "walkload", dirname, "walkload", {8, 24}, 10)


tribes:new_worker_type {
   name = "atlanteans_charcoal_burner",
   -- TRANSLATORS: This is a worker name used in lists of workers
   descname = _"Charcoal Burner",
   icon = dirname .. "menu.png",
   vision_range = 2,

   buildcost = {
		atlanteans_carrier = 1
	},

	-- TRANSLATORS: Helptext for a worker: Charcoal Burner
   helptext = _"Burns coal.",
   animations = animations,
}
