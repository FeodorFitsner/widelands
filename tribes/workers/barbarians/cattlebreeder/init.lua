dirname = path.dirname(__file__)

animations = {
   idle = {
      pictures = { dirname .. "idle_\\d+.png" },
      hotspot = { 3, 21 },
      fps = 10
   }
}
add_worker_animations(animations, "walk", dirname, "walk", {42, 30}, 20)
add_worker_animations(animations, "walkload", dirname, "walk", {42, 30}, 20)


tribes:new_worker_type {
   name = "barbarians_cattlebreeder",
   -- TRANSLATORS: This is a worker name used in lists of workers
   descname = _"Cattle Breeder",
   vision_range = 2,

   buildcost = {
		barbarians_carrier = 1
	},

	-- TRANSLATORS: Helptext for a worker: Cattle Breeder
   helptext = _"Breeds strong oxen for adding them to the transportation system.",
   animations = animations,
}
