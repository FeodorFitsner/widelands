dirname = path.dirname(__file__)

tribes:new_ware_type{
   name = "ration",
   -- TRANSLATORS: This is a ware name used in lists of wares
   descname = _"Ration",
   -- TRANSLATORS: mass description, e.g. 'The economy needs ...'
   genericname = _"rations",
   default_target_quantity = {
		barbarians = 20,
		empire = 20
	},
   preciousness = {
		barbarians = 5,
		empire = 4
	},
   helptext = {
		-- TRANSLATORS: Helptext for a ware: Ration
		default = _"A small bite to keep miners strong and working. The scout also consumes rations on his scouting trips.",
		-- TRANSLATORS: Helptext for a ware: Ration
		barbarians = _"Rations are produced in a tavern, an inn or a big inn out of fish or meat or pitta bread.",
		-- TRANSLATORS: Helptext for a ware: Ration
		empire = _"Rations are produced in a tavern out of fish or meat or bread."
   },
   animations = {
      idle = {
         pictures = { dirname .. "idle.png" },
         hotspot = { 5, 5 },
      },
   }
}
