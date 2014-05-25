-- The Barbarian Granite Mine

include "scripting/formatting.lua"
include "scripting/format_help.lua"

set_textdomain("tribe_barbarians")

return {
   func = function(building_description)
	return

	--Lore Section
	building_help_lore_string("barbarians", building_description, _[[‘I can handle tons of granite, man, but no more of your vain prattle.’]], _[[This phrase was the reply Rimbert the miner – later known as Rimbert the loner – gave, when he was asked to remain seated on an emergency meeting at Stonford in the year of the great flood. <br> The same man had all the 244 granite blocks ready only a week later, and they still fortify the city’s levee.]]) ..

	--General Section
	building_help_general_string("barbarians", building_description, "raw_stone",
		_"Carve raw stone out of the rock in mountain terrain.",
		_"This mine exploits all of the resource down to the deepest level. But even after having done so, it will still have a %s chance of finding some more raw stone.":bformat("5%") .. "<br>" .. _" It cannot be upgraded.",
		"2") ..

	--Dependencies
	building_help_dependencies_production("barbarians", building_description, false, true) ..

	--Workers Section
	building_help_crew_string("barbarians", building_description) ..

	--Building Section
	building_help_building_section("barbarians", building_description) ..

	--Production Section
		rt(h2(_"Production")) ..
		text_line(_"Performance:", _"If the food supply is steady, this mine can produce raw stone in %s on average.":format("20s"))
  end
}
