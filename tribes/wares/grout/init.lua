dirname = path.dirname(__file__)

tribes:new_ware_type {
   msgctxt = "ware",
   name = "grout",
   -- TRANSLATORS: This is a ware name used in lists of wares
   descname = pgettext("ware", "Grout"),
   -- TRANSLATORS: mass description, e.g. 'The economy needs ...'
   genericname = pgettext("ware", "grout"),
   directory = dirname,
   icon = dirname .. "menu.png",
   default_target_quantity = {
		barbarians = 10
	},
   preciousness = {
		barbarians = 5
	},

   animations = {
      idle = {
         pictures = { dirname .. "idle.png" },
         hotspot = { 5, 12 },
      },
   }
}
