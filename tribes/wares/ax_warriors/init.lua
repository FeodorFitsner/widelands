dirname = path.dirname(__file__)

tribes:new_ware_type{
   name = "ax_warriors",
   -- TRANSLATORS: This is a ware name used in lists of wares
   descname = _"Warrior’s Ax",
   -- TRANSLATORS: mass description, e.g. 'The economy needs ...'
   genericname = _"warrior’s axes",
   default_target_quantity = {
		barbarians = 1
	},
   preciousness = {
		barbarians = 1
	},
   helptext = {
		-- TRANSLATORS: Helptext for a ware: Warrior’s Ax
		barbarians = _"The warrior’s axe is the most dangerous of all Barbarian weapons. Only a few warriors ever were able to handle those huge and powerful axes. It is produced in a war mill and used – together with food – in a training camp to train soldiers from attack level 4 to level 5."
   },
   animations = {
      idle = {
         pictures = { dirname .. "idle.png" },
         hotspot = { 7, 7 },
      },
   }
}
