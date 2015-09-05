dirname = path.dirname(__file__)

animations = {
   idle = {
      pictures = path.list_directory(dirname, "idle_\\d+.png"),
      hotspot = { 14, 22 }
   },
   releasein = {
      pictures = path.list_directory(dirname, "releasein_\\d+.png"),
      hotspot = { 15, 22 },
      fps = 5
   },
   releaseout = {
      pictures = path.list_directory(dirname, "releaseout_\\d+.png"),
      hotspot = { 15, 22 },
      fps = 5
   }
}
add_worker_animations(animations, "walk", dirname, "walk", {15, 22}, 10)
add_worker_animations(animations, "walkload", dirname, "walkload", {15, 22})


tribes:new_worker_type {
   msgctxt = "barbarians_worker",
   name = "barbarians_gamekeeper",
   -- TRANSLATORS: This is a worker name used in lists of workers
   descname = pgettext("barbarians_worker", "Gamekeeper"),
   directory = dirname,
   icon = dirname .. "menu.png",
   vision_range = 2,

   buildcost = {
		barbarians_carrier = 1
	},

	programs = {
		release = {
			"setbobdescription wildboar stag sheep",
			"findspace size:any radius:3",
			"walk coords",
			"animation releasein 2000",
			"create_bob",
			"animation releaseout 2000",
			"return"
		}
	},

   animations = animations,
}
