-- The Barbarian Cattle Farm
include "scripting/formatting.lua"
set_textdomain("tribes")
include "tribes/scripting/format_help.lua"

return {
   func = function(building_description)
	return

	--Lore Section
	building_help_lore_string("barbarians", building_description, _[[Text needed]], _[[Source needed]]) ..

	--General Section
	building_help_general_string("barbarians", building_description,
		_"Breeds strong oxen for adding them to the transportation system.") ..

	--Dependencies
	building_help_dependencies_production("barbarians", building_description) ..

	--Workers Section
	building_help_crew_string("barbarians", building_description) ..

	--Building Section
	building_help_building_section("barbarians", building_description) ..

	--Production Section
	building_help_production_section(_"If all needed wares are delivered in time, this building can produce an ox in %s on average.":bformat(
		ngettext("%d second", "%d seconds", 30):bformat(30)
	))
  end
}
