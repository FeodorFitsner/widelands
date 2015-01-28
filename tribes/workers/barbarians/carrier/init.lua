dirname = path.dirname(__file__)

animations = {
   idle = {
      pictures = { dirname .. "idle_\\d+.png" },
      hotspot = { 14, 21 },
      fps = 5
   }
}
add_worker_animations(animations, "walk", dirname, "walk", {9, 19}, 10)
add_worker_animations(animations, "walkload", dirname, "walkload", {7, 22}, 10)


tribes:new_carrier_type {
   name = "barbarians_carrier",
   -- TRANSLATORS: This is a worker name used in lists of workers
   descname = _"Carrier",
   vision_range = 2,

	-- TRANSLATORS: Helptext for a worker: Carrier
   helptext = _"Carries items along your roads.",
   animations = animations,
}
