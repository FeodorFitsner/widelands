dirname = path.dirname(__file__)

tribes:new_ware_type{
   name = "helmet_mask",
   -- TRANSLATORS: This is a ware name used in lists of wares
   descname = _"Mask",
   -- TRANSLATORS: mass description, e.g. 'The economy needs ...'
   genericname = _"masks",
   default_target_quantity = {
		barbarians = 1
	},
   preciousness = {
		barbarians = 1
	},
   helptext = {
		-- TRANSLATORS: Helptext for a ware: Mask
		barbarians = _"This is the most enhanced Barbarian armor. It is produced in the helm smithy and used in the training camp – together with food – to train soldiers from health level 2 to level 3."
   },
   animations = {
      idle = {
         pictures = { dirname .. "idle.png" },
         hotspot = { 7, 10 },
      },
   }
}
