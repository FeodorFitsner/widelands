-- The Imperial Armor Smithy

include "scripting/formatting.lua"
include "scripting/format_help.lua"

set_textdomain("tribe_empire")

return {
   func = function(building_description)
	return

	--Lore Section
	-- TODO rename pictures building_help_lore_string("empire", building_description, _[[Text needed]], _[[Source needed]]) ..

	--General Section
	building_help_general_string("empire", building_description, "helm",
		_[[Text needed]]) ..

	--Dependencies
	building_help_dependencies_production("empire", building_description) ..

	--Workers Section
	building_help_crew_string("empire", building_description) ..

	--Building Section
	building_help_building_section("empire", building_description) ..

	--Production Section
	building_help_production_section(_[[Calculation needed]])
   end
}
