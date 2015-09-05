dirname = path.dirname(__file__)

animations = {
   idle = {
      pictures = path.list_directory(dirname, "idle_\\d+.png"),
      hotspot = { 3, 22 },
   }
}
add_worker_animations(animations, "walk", dirname, "walk", {6, 22}, 10)
add_worker_animations(animations, "walkload", dirname, "walkload", {9, 23}, 10)


tribes:new_worker_type {
   msgctxt = "barbarians_worker",
   name = "barbarians_blacksmith_master",
   -- TRANSLATORS: This is a worker name used in lists of workers
   descname = pgettext("barbarians_worker", "Master Blacksmith"),
   icon = dirname .. "menu.png",
   vision_range = 2,

	-- TRANSLATORS: Helptext for a worker: Master Blacksmith
   helptext = pgettext("barbarians_worker", "Produces weapons for soldiers and tools for workers."),
   animations = animations,
}
