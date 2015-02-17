dirname = path.dirname(__file__)

tribes:new_ware_type{
   name = "hunting_bow",
   -- TRANSLATORS: This is a ware name used in lists of wares
   descname = _"Hunting Bow",
   -- TRANSLATORS: mass description, e.g. 'The economy needs ...'
   genericname = _"hunting bows",
   icon = dirname .. "menu.png",
   default_target_quantity = {
		atlanteans = 1
	},
   preciousness = {
		atlanteans = 0
	},
   helptext = {
		-- TRANSLATORS: Helptext for a ware: Hunting Bow
		atlanteans = _"This bow is used by the Atlantean hunter. It is produced by the toolsmith."
   },
   animations = {
      idle = {
         pictures = { dirname .. "idle.png" },
         hotspot = { 9, 16 },
      },
   }
}
