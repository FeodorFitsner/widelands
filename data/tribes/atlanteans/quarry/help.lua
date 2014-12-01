-- The Atlantean Quarry

include "data/scripting/formatting.lua"
include "data/tribes/scripting/format_help.lua"

set_textdomain("tribe_atlanteans")

return {
   func = function(building_description)
	return

	--Lore Section
	building_help_lore_string("atlanteans", building_description, _[[Text needed]],_[[Source needed]]) ..

	--General Section
	building_help_general_string("atlanteans", building_description,
		_"Carves stones out of rocks in the vicinity.", _"The quarry needs stones to cut within the working radius.") ..

	--Dependencies
	building_help_dependencies_production("atlanteans", building_description, true) ..

	--Workers Section
	building_help_crew_string("atlanteans", building_description) ..

	--Building Section
	building_help_building_section("atlanteans", building_description) ..

	--Production Section
	building_help_production_section(_[[Calculation needed]])
   end
}
