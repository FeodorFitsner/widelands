dirname = path.dirname(__file__)

animations = {
   idle = {
      pictures = { dirname .. "idle_\\d+.png" },
      hotspot = { 6, 24 },
      fps = 10
   },
   hacking = {
      pictures = { dirname .. "hacking_\\d+.png" },
      hotspot = { 23, 23 },
      fps = 10
   }
}
add_worker_animations(animations, "walk", dirname, "walk", {9, 22}, 10)
add_worker_animations(animations, "walkload", dirname, "walkload", {9, 22}, 10)


tribes:new_worker_type {
   name = "empire_lumberjack",
   -- TRANSLATORS: This is a worker name used in lists of workers
   descname = _"Lumberjack",
   vision_range = 2,

   buildcost = {
		empire_carrier = 1,
		felling_ax = 1
	},

	programs = {
		chop = {
			"findobject attrib:tree radius:10",
			"walk object",
			"playFX ../../../sound/woodcutting/fast_woodcutting 250",
			"animation hacking 10000",
			"playFX ../../../sound/spoken/timber 156",
			"object fall",
			"animation idle 2000",
			"createware log",
			"return"
		}
	},

	-- TRANSLATORS: Helptext for a worker: Lumberjack
   helptext = _"Fells trees.",
   animations = animations,
}
