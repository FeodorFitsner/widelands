dirname = path.dirname(__file__)

tribes:new_productionsite_type {
   msgctxt = "empire_building",
   name = "empire_bakery",
   -- TRANSLATORS: This is a building name used in lists of buildings
   descname = pgettext("empire_building", "Bakery"),
   helptext_script = dirname .. "helptexts.lua",
   icon = dirname .. "menu.png",
   size = "medium",

   buildcost = {
      log = 2,
      planks = 2,
      granite = 3
   },
   return_on_dismantle = {
      planks = 1,
      granite = 2
   },

   animations = {
      idle = {
         pictures = path.list_files(dirname .. "idle_??.png"),
         hotspot = { 42, 65 },
      },
      build = {
         pictures = path.list_files(dirname .. "build_??.png"),
         hotspot = { 42, 65 },
      },
      working = {
         pictures = path.list_files(dirname .. "working_??.png"),
         hotspot = { 43, 65 },
         fps = 2
      },
   },

   aihints = {
      prohibited_till = 600,
      forced_after = 700
   },

   working_positions = {
      empire_baker = 1
   },

   inputs = {
      flour = 6,
      water = 6
   },
   outputs = {
      "empire_bread"
   },

   programs = {
      work = {
         -- TRANSLATORS: Completed/Skipped/Did not start baking bread because ...
         descname = _"baking bread",
         actions = {
            "sleep=15000",
            "return=skipped unless economy needs empire_bread",
            "consume=flour water",
            "animate=working 15000",
            "produce=empire_bread"
         }
      },
   },
}
