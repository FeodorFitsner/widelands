dirname = path.dirname(__file__)

tribes:new_constructionsite_type {
   name = "constructionsite",
   -- TRANSLATORS: This is a name used in lists of buildings for buildings under construction
   descname = _"Construction Site",
   size = "small", -- Dummy; overridden by building size
   buildable = false, -- Counterintuitive, but correct: player can't build constructionsite
   vision_range = 2,

   helptexts = {
		-- TRANSLATORS: Lore helptext for a building
		lore = _"Text needed",
		-- TRANSLATORS: Lore author helptext for a building
		lore_author = _"Source needed",
		-- TRANSLATORS: Purpose helptext for a building
		purpose = _"Text needed",
		-- #TRANSLATORS: Note helptext for a building
		note = "",
		-- TRANSLATORS: Performance helptext for a building
		performance = _"Calculation needed"
   }

   animations = {
		-- The constructionsite is a mess. Not nice and clean, but rather some
		-- logs lying around on piles, maybe some tools.
		idle = {
			pictures = { dirname .. "idle_\\d+.png" },
			hotspot = { 5, 5 },
		},
		idle_with_worker = {
			pictures = { dirname .. "idle_with_worker_\\d+.png" },
			hotspot = { 33, 36 },
		}
	},
}
