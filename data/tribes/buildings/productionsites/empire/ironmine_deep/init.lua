dirname = path.dirname(__file__)

tribes:new_productionsite_type {
   msgctxt = "empire_building",
   name = "empire_ironmine_deep",
   -- TRANSLATORS: This is a building name used in lists of buildings
   descname = pgettext("empire_building", "Deep Iron Mine"),
   helptext_script = dirname .. "helptexts.lua",
   icon = dirname .. "menu.png",
   size = "mine",

   enhancement_cost = {
      log = 4,
      planks = 2
   },
   return_on_dismantle_on_enhanced = {
      log = 2,
      planks = 1
   },

   animations = {
      idle = {
         pictures = path.list_files(dirname .. "idle_??.png"),
         hotspot = { 49, 61 },
      },
      working = {
         pictures = path.list_files(dirname .. "working_??.png"),
         hotspot = { 49, 61 },
         fps = 10
      },
      empty = {
         pictures = path.list_files(dirname .. "empty_??.png"),
         hotspot = { 49, 61 },
      },
   },

   aihints = {
      mines = "gold"
   },

   working_positions = {
      empire_miner = 1,
      empire_miner_master = 1
   },

   inputs = {
      meal = 6,
      beer = 6
   },
   outputs = {
      "iron_ore"
   },

   programs = {
      work = {
         -- TRANSLATORS: Completed/Skipped/Did not start mining iron because ...
         descname = _"mining iron",
         actions = {
            "sleep=43000",
            "return=skipped unless economy needs iron_ore",
            "consume=meal beer",
            "animate=working 18000",
            "mine=iron 2 100 5 2",
            "produce=iron_ore:2",
            "animate=working 18000",
            "mine=iron 2 100 5 2",
            "produce=iron_ore:2"
         }
      },
   },
   out_of_resource_notification = {
      -- Translators: Short for "Out of ..." for a resource
      title = _"No Iron",
      heading = _"Main Iron Vein Exhausted",
      message =
         pgettext("empire_building", "This iron mine’s main vein is exhausted. Expect strongly diminished returns on investment. This mine can’t be enhanced any further, so you should consider dismantling or destroying it."),
   },
}
