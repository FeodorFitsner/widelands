dirname = path.dirname(__file__)

animations = {
   idle = {
      pictures = { dirname .. "idle_\\d+.png" },
      hotspot = { 14, 20 },
      fps = 10
   }
}
add_worker_animations(animations, "walk", dirname, "donkey_walk", {14, 25}, 10)
add_worker_animations(animations, "walkload", dirname, "donkey_walk", {14, 25}, 10)


tribes:new_carrier_type {
   name = "empire_donkey",
   -- TRANSLATORS: This is a worker name used in lists of workers
   descname = _"Donkey",
   icon = dirname .. "menu.png",
   vision_range = 2,

   default_target_quantity = 10,
   ware_hotspot =  { -2, 8 }

	-- TRANSLATORS: Helptext for a worker: Donkey
   helptext = _"Donkeys help to carry items along busy roads. They are reared in a donkey farm.",
   animations = animations,
}
