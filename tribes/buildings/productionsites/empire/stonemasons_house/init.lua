dirname = path.dirname(__file__)

tribes:new_productionsite_type {
   name = "empire_stonemasons_house",
   -- TRANSLATORS: This is a building name used in lists of buildings
   descname = _"Stonemason’s House",
   icon = dirname .. "menu.png",
   size = "medium",

   buildcost = {
		log = 1,
		planks = 1,
		granite = 1,
		marble = 3 -- Someone who works on marble should like marble.
	},
	return_on_dismantle = {
		granite = 1,
		marble = 2
	},

   helptexts = {
		-- TRANSLATORS: Lore helptext for a building
		lore = _"Text needed",
		-- TRANSLATORS: Lore author helptext for a building
		lore_author = _"Source needed",
		-- TRANSLATORS: Purpose helptext for a building
		purpose = _"Text needed",
		-- TRANSLATORS: Note helptext for a building
		note = "",
		-- TRANSLATORS: Performance helptext for a building
		performance = _"Calculation needed"
   }

   animations = {
		idle = {
			pictures = { dirname .. "idle_\\d+.png" },
			hotspot = { 58, 61 },
		},
		working = {
			pictures = { dirname .. "idle_\\d+.png" }, -- TODO(GunChleoc): No animation yet.
			hotspot = { 58, 61 },
		},
	},

   aihints = {
		forced_after = 300,
		prohibited_till = 120
   },

	working_positions = {
		empire_stonemason = 1
	},

   inputs = {
		marble = 6
	},
   outputs = {
		"marble_column"
   },

	programs = {
		work = {
			-- TRANSLATORS: Completed/Skipped/Did not start sculpting a marble column because ...
			descname = _"sculpting a marble column",
			actions = {
				"sleep=50000",
				"return=skipped unless economy needs marble_column",
				"consume=marble:2",
				"playFX=../../../sound/stonecutting/stonemason 192",
				"animate=working 32000",
				"produce=marble_column"
			}
		},
	},
}
