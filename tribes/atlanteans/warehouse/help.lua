-- The Atlantean Warehouse

include "scripting/formatting.lua"
include "scripting/format_help.lua"

set_textdomain("tribe_atlanteans")

return {
   func = function(building_description)
	return

	--Lore Section
	building_help_lore_string("atlanteans", building_description, _[[Text needed]], _[[Source needed]]) ..
	--General Section
	building_help_general_string("atlanteans", building_description, "log",
		_"Warehouses store soldiers, wares and tools.") ..

	--Building Section
	building_help_building_section("atlanteans", building_description)
   end
}
