dirname = path.dirname(__file__)

tribes:new_ware_type{
   name = "trident_double",
   -- TRANSLATORS: This is a ware name used in lists of wares
   descname = _"Double Trident",
   -- TRANSLATORS: mass description, e.g. 'The economy needs ...'
   genericname = _"double tridents",
   default_target_quantity = {
		atlanteans = 1
	},
   preciousness = {
		atlanteans = 1
	},
   helptext = {
		-- TRANSLATORS: Helptext for a ware: Double Trident
		atlanteans = _"The double trident is one of the best tridents produced by the Atlantean weapon smithy. It is used in a dungeon – together with food – to train soldiers from attack level 2 to level 3."
	},
   animations = {
      idle = {
         pictures = { dirname .. "idle.png" },
         hotspot = { 10, 17 },
      },
   }
}
