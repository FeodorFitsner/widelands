dirname = path.dirname(__file__)

animations = {
   idle = {
      pictures = { dirname .. "idle_\\d+.png" },
      hotspot = { 13, 21 },
      fps = 10
   },
   work = {
      pictures = { dirname .. "work_\\d+.png" },
      hotspot = { 11, 21 },
      fps = 10
   }
}
add_worker_animations(animations, "walk", dirname, "walk", {11, 23}, 10)
add_worker_animations(animations, "walkload", dirname, "walk", {11, 23}, 10)


tribes:new_worker_type {
   name = "empire_builder",
   -- TRANSLATORS: This is a worker name used in lists of workers
   descname = _"Builder",

   buildcost = {
		empire_carrier = 1,
		hammer = 1
	},

	-- TRANSLATORS: Helptext for a worker: Builder
   helptext = _"Works at construction sites to raise new buildings.",
   animations = animations,
}
