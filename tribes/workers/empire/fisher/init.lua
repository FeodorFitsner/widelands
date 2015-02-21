dirname = path.dirname(__file__)

animations = {
   idle = {
      pictures = path.list_directory(dirname, "idle_\\d+.png"),
      hotspot = { 7, 38 },
   },
   fishing = {
      pictures = path.list_directory(dirname, "fishing_\\d+.png"),
      hotspot = { 9, 39 },
      fps = 10
   }
}
add_worker_animations(animations, "walk", dirname, "walk", {10, 38}, 20)
add_worker_animations(animations, "walkload", dirname, "walk", {10, 38}, 20)


tribes:new_worker_type {
   name = "empire_fisher",
   -- TRANSLATORS: This is a worker name used in lists of workers
   descname = _"Fisher",
   icon = dirname .. "menu.png",
   vision_range = 2,

   buildcost = {
		empire_carrier = 1,
		fishing_rod = 1
	},

	programs = {
		fish = {
			"findspace size:any radius:7 resource:fish",
			"walk coords",
			"playFX sound/fisher/fisher_throw_net 192",
			"mine fish 1",
			"animation fishing 3000", -- Play a fishing animation
			"playFX sound/fisher/fisher_pull_net 192",
			"createware fish",
			"return"
		}
	},

	-- TRANSLATORS: Helptext for a worker: Fisher
   helptext = _"Catches fish in the sea.",
   animations = animations,
}
