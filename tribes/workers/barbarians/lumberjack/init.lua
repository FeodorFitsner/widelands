dirname = path.dirname(__file__)

animations = {
   idle = {
      pictures = path.list_directory(dirname, "idle_\\d+.png"),
      hotspot = { 5, 23 }
   },
   hacking = {
      pictures = path.list_directory(dirname, "hacking_\\d+.png"),
      hotspot = { 19, 17 },
      fps = 10
   }
}
add_worker_animations(animations, "walk", dirname, "walk", {10, 22}, 10)
add_worker_animations(animations, "walkload", dirname, "walkload", {10, 21}, 10)


tribes:new_worker_type {
   name = "barbarians_lumberjack",
   -- TRANSLATORS: This is a worker name used in lists of workers
   descname = _"Lumberjack",
   icon = dirname .. "menu.png",
   vision_range = 2,

   buildcost = {
		barbarians_carrier = 1,
		felling_ax = 1
	},

	programs = {
		chop = {
			"findobject attrib:tree radius:10",
			"walk object",
			"playFX ../../../sound/woodcutting/woodcutting 255",
			"animation hacking 10000",
			"playFX ../../../sound/spoken/timber 192",
			"object fall",
			"animation idle 2000",
			"createware log",
			"return"
		}
	},

	-- TRANSLATORS: Helptext for a worker: Helmsmith
   helptext = _"Fells trees.",
   animations = animations,
}
