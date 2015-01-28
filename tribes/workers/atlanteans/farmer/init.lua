dirname = path.dirname(__file__)

animations = {
   idle = {
      pictures = { dirname .. "idle_\\d+.png" },
      hotspot = { 10, 23 },
   },
   planting = {
      pictures = { dirname .. "plant_\\d+.png" },
      hotspot = { 13, 32 },
      fps = 10
   },
   harvesting = {
      pictures = { dirname .. "harvest_\\d+.png" },
      hotspot = { 18, 32 },
      fps = 10
   },
   gathering = {
      pictures = { dirname .. "gather_\\d+.png" },
      hotspot = { 10, 34 },
      fps = 5
   }
}
add_worker_animations(animations, "walk", dirname, "walk", {18, 23}, 10)
add_worker_animations(animations, "walkload", dirname, "walk", {18, 23}, 10)


tribes:new_worker_type {
   name = "atlanteans_farmer",
   -- TRANSLATORS: This is a worker name used in lists of workers
   descname = _"Farmer",
   vision_range = 2,

   buildcost = {
		atlanteans_carrier = 1,
		scythe = 1
	},

	programs = {
		plant = {
			"findspace size:any radius:2",
			"walk coords",
			"animation planting 4000",
			"plant tribe:cornfield_tiny",
			"animation planting 4000",
			"return"
		},
		harvest = {
			"findobject attrib:ripe_corn radius:2",
			"walk object",
			"playFX ../../../sound/farm/scythe 220",
			"animation harvesting 10000",
			"object harvest",
			"animation gathering 4000",
			"createware corn",
			"return"
		}
	},

	-- TRANSLATORS: Helptext for a worker: Farmer
   helptext = _"Plants and harvests cornfields.",
   animations = animations,
}
