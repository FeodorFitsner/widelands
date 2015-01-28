dirname = path.dirname(__file__)

animations = {
   idle = {
      pictures = { dirname .. "idle_\\d+.png" },
      hotspot = { 10, 23 },
   },
   planting = {
      pictures = { dirname .. "plant_\\d+.png" },
      hotspot = { 14, 25 },
      fps = 10
   },
   harvesting = {
      pictures = { dirname .. "harvest_\\d+.png" },
      hotspot = { 19, 23 },
      fps = 10
   },
   gathering = {
      pictures = { dirname .. "gather_\\d+.png" },
      hotspot = { 9, 23 },
      fps = 5
   }
}
add_worker_animations(animations, "walk", dirname, "walk", {18, 23}, 10)
add_worker_animations(animations, "walkload", dirname, "walkload", {16, 23}, 10)


tribes:new_worker_type {
   name = "barbarians_farmer",
   -- TRANSLATORS: This is a worker name used in lists of workers
   descname = _"Farmer",
   vision_range = 2,

   buildcost = {
		barbarians_carrier = 1,
		scythe = 1
	},

	programs = {
		plant = {
			"findspace size:any radius:2 space",
			"walk coords",
			"animation planting 4000",
			"plant tribe:field_tiny",
			"animation planting 4000",
			"return"
		},
		harvest = {
			"findobject attrib:wheatfield radius:2",
			"walk object",
			"playFX ../../../sound/farm/scythe 220",
			"animation harvesting 10000",
			"object harvest",
			"animation gathering 4000",
			"createware wheat",
			"return"
		}
	},

	-- TRANSLATORS: Helptext for a worker: Farmer
   helptext = _"Plants fields.",
   animations = animations,
}
