dirname = path.dirname(__file__)

animations = {
   idle = {
      pictures = { dirname .. "idle_\\d+.png" },
      hotspot = { 9, 24 },
   },
   planting = {
      pictures = { dirname .. "plant_\\d+.png" },
      hotspot = { 13, 32 },
      fps = 10
   },
   harvesting = {
      pictures = { dirname .. "harvest_\\d+.png" },
      hotspot = { 13, 32 },
      fps = 10
   },
   gathering = {
      pictures = { dirname .. "gather_\\d+.png" },
      hotspot = { 13, 32 },
      fps = 10
   }
}
add_worker_animations(animations, "walk", dirname, "walk", {13, 24}, 10)
add_worker_animations(animations, "walkload", dirname, "walk", {13, 24}, 10)


tribes:new_worker_type {
   name = "atlanteans_blackroot_farmer",
   -- TRANSLATORS: This is a worker name used in lists of workers
   descname = _"Blackroot Farmer",
   icon = dirname .. "menu.png",
   vision_range = 2,

   buildcost = {
		atlanteans_carrier = 1,
		shovel = 1
	},

	programs = {
		plant = {
			"findspace size:any radius:2",
			"walk coords",
			"animation planting 4000",
			"plant tribe:blackrootfield_tiny",
			"animation planting 4000",
			"return"
		},
		harvest = {
			"findobject attrib:ripe_blackroot radius:2",
			"walk object",
			"animation harvesting 10000",
			"object harvest",
			"animation gathering 2000",
			"createware blackroot",
			"return"
		}
	},

	-- TRANSLATORS: Helptext for a worker: Blackroot Farmer
   helptext = _"Plants and harvests blackroot.",
   animations = animations,
}
