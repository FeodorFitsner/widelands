dirname = path.dirname(__file__)

tribes:new_productionsite_type {
   msgctxt = "barbarians_building",
   name = "barbarians_scouts_hut",
   -- TRANSLATORS: This is a building name used in lists of buildings
   descname = pgettext("barbarians_building", "Scout’s Hut"),
   helptext_script = dirname .. "helptexts.lua",
   icon = dirname .. "menu.png",
   size = "small",

   buildcost = {
      log = 2,
      granite = 1
   },
   return_on_dismantle = {
      log = 1,
      granite = 1
   },

   animations = {
      idle = {
         pictures = path.list_files(dirname .. "idle_??.png"),
         hotspot = { 45, 92 },
      },
      build = {
         pictures = path.list_files(dirname .. "build_??.png"),
         hotspot = { 45, 92 },
      },
      unoccupied = {
         pictures = path.list_files(dirname .. "unoccupied_??.png"),
         hotspot = { 45, 92 },
      },
   },

   aihints = {},

   working_positions = {
      barbarians_scout = 1
   },

   inputs = {
      ration = 2
   },

   programs = {
      work = {
         -- TRANSLATORS: Completed/Skipped/Did not start scouting because ...
         descname = _"scouting",
         actions = {
            "sleep=30000",
            "consume=ration",
            "worker=scout"
         }
      },
   },
}
