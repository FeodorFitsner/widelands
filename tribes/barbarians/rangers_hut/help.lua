-- The Barbarian Ranger's Hut

include "scripting/formatting.lua"
include "scripting/format_help.lua"

set_textdomain("tribe_barbarians")

return {
   func = function(building_description)
	return

	--Lore Section
	--"Wer zwei Halme dort wachsen lässt, wo sonst nur einer wächst, der ist größer als der größte Feldherr!" – Friedrich der Große
	-- TODO crashes
--	building_help_lore_string("barbarians", building_description, _[[‘He who can grow two trees where normally only one will grow exceeds the most important general!’]],_[[Chat’Karuth in a conversation with a Ranger]]) ..

	--General Section
	building_help_general_string("barbarians", building_description, "log",
		_"Plants trees in the surrounding area.",
		_"The ranger’s hut needs free space within the working radius to plant the trees.", "5") ..

	--Dependencies
	rt(h2(_"Dependencies")) ..

	--Workers Section
	building_help_crew_string("barbarians", building_description, {"ranger"}, "shovel") ..

	--Building Section
	building_help_building_section("barbarians", building_description) ..

	--Production Section
		rt(h2(_"Production")) ..
		text_line(_"Performance:", _"The ranger needs %s to plant a tree, not counting the time he needs to reach the destination and go home again.":bformat("5s"))
   end
}
