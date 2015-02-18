dirname = path.dirname(__file__)

animations = {
   idle = {
      pictures = path.list_directory(dirname, "idle_\\d+.png"),
      hotspot = { 8, 22 }
   },
   sawing = {
      pictures = path.list_directory(dirname, "sawing_\\d+.png"),
      hotspot = { 22, 19 },
      fps = 10
   }
}
add_worker_animations(animations, "walk", dirname, "walk", {16, 31}, 10)
add_worker_animations(animations, "walkload", dirname, "walkload", {13, 29}, 10)


tribes:new_worker_type {
   name = "atlanteans_woodcutter",
   -- TRANSLATORS: This is a worker name used in lists of workers
   descname = _"Woodcutter",
   icon = dirname .. "menu.png",
   vision_range = 2,

   buildcost = {
		atlanteans_carrier = 1,
		saw = 1
	},

	programs = {
		harvest = {
			"findobject attrib:tree radius:10",
			"walk object",
			"playFX ../../../sound/sawmill/sawmill 230",
			"animation sawing 10000",
			"object fall",
			"animation idle 2000",
			"createware log",
			"return"
		}
	}

	-- TRANSLATORS: Helptext for a worker: Woodcutter
   helptext = _"Fells trees.",
   animations = animations,
}
