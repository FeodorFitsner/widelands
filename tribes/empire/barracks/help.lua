-- The Imperial Barracks

include "scripting/formatting.lua"
include "scripting/format_help.lua"

set_textdomain("tribe_empire")

return {
   func = function(building_description)
	return

	--Lore Section
	building_help_lore_string("empire", building_description, _[[Text needed]], _[[Source needed]]) ..
	--General Section
	building_help_general_string("empire", building_description, "soldier",
		_"Garrisons soldiers to expand your territory.",
		_"If you’re low on soldiers to occupy new military sites, use the downward arrow button to decrease the capacity. You can also click on a soldier to send him away.") ..

	--Building Section
	building_help_building_section("empire", building_description)
   end
}
