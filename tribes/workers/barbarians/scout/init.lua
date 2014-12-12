dirname = path.dirname(__file__)

animations = {
   idle = {
      pictures = { dirname .. "idle_\\d+.png" },
      hotspot = { 4, 23 },
      fps = 10
   }
}
add_worker_animations(animations, "walk", dirname, "walk", {9, 25}, 10)
add_worker_animations(animations, "walkload", dirname, "walk", {9, 25}, 10)


tribes:new_worker_type {
   name = "barbarians_scout",
   -- TRANSLATORS: This is a worker name used in lists of workers
   descname = _"Scout",

   buildcost = {
		barbarians_carrier = 1
	},

	programs = {
		scout = {
			"scout 15 75000", -- radius 15, 75 seconds until return
			"return"
		}
	},

	-- TRANSLATORS: Helptext for a worker: Scout
   helptext = _"Scouts like Scotty the scout scouting unscouted areas in a scouty fashion.", -- (c) WiHack Team 02.01.2010
   animations = animations,
}
