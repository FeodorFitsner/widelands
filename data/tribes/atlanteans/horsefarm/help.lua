-- The Atlantean Horse Farm

include "data/scripting/formatting.lua"
include "data/tribes/scripting/format_help.lua"

set_textdomain("tribe_atlanteans")

return {
   func = function(building_description)
	return

	--Lore Section
	building_help_lore_string("atlanteans", building_description, _[[Text needed]], _[[Source needed]]) ..

	--General Section
	building_help_general_string("atlanteans", building_description,
		_"Breeds horses for adding them to the transportation system.") ..

	--Dependencies
	building_help_dependencies_production("atlanteans", building_description) ..

	--Workers Section
	building_help_crew_string("atlanteans", building_description) ..

	--Building Section
	building_help_building_section("atlanteans", building_description) ..

	--Production Section
	building_help_production_section(_[[Calculation needed]])
  end
}
