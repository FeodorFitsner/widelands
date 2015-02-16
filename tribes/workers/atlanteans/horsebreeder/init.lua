dirname = path.dirname(__file__)

animations = {
   idle = {
      pictures = { dirname .. "idle_\\d+.png" },
      hotspot = { 8, 23 },
      fps = 10
   }
}
add_worker_animations(animations, "walk", dirname, "walk", {35, 28}, 10)
add_worker_animations(animations, "walkload", dirname, "walk", {35, 28}, 10)


tribes:new_worker_type {
   name = "atlanteans_horsebreeder",
   -- TRANSLATORS: This is a worker name used in lists of workers
   descname = _"Horse Breeder",
   icon = dirname .. "menu.png",
   vision_range = 2,

   buildcost = {
		atlanteans_carrier = 1
	},

	-- TRANSLATORS: Helptext for a worker: Horse Breeder
   helptext = _"Breeds the strong Atlantean horses for adding them to the transportation system.",
   animations = animations,
}
