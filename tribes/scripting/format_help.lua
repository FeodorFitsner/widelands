-- TODO(GunChleoc): get resi_00.png from C++

set_textdomain("tribes")

--  =======================================================
--  *************** Basic helper functions ****************
--  =======================================================

-- RST
-- .. function:: image_line(image, count[, text = nil])
--
--    Aligns the image to a row on the right side with text on the left.
--
--    :arg image: the picture to be aligned to a row.
--    :arg count: length of the picture row.
--    :arg text: if given the text aligned on the left side, formatted via
--       formatting.lua functions.
--    :returns: the text on the left and a picture row on the right.
--
function image_line(image, count, text)
	local imgs={}
	for i=1,count do
		imgs[#imgs + 1] = image
	end
	local imgstr = table.concat(imgs, ";")

	if text then
		return rt("image=" .. imgstr .. " image-align=right", "  " .. text)
	else
		return rt("image=" .. imgstr .. " image-align=right", "")
	end
end

-- RST
-- .. function text_line(t1, t2[, imgstr = nil])
--
--    Creates a line of h3 formatted text followed by normal text and an image.
--
--    :arg t1: text in h3 format.
--    :arg t2: text in p format.
--    :arg imgstr: image aligned right.
--    :returns: header followed by normal text and image.
--
function text_line(t1, t2, imgstr)
	if imgstr then
		return "<rt text-align=left image=" .. imgstr .. " image-align=right><p font-size=13 font-color=D1D1D1>" ..  t1 .. "</p><p line-spacing=3 font-size=12>" .. t2 .. "<br></p><p font-size=8> <br></p></rt>"
	else
		return "<rt text-align=left><p font-size=13 font-color=D1D1D1>" ..  t1 .. "</p><p line-spacing=3 font-size=12>" .. t2 .. "<br></p><p font-size=8> <br></p></rt>"
	end
end


--  =======================================================
--  ********** Helper functions for dependencies **********
--  =======================================================

-- RST
-- format_help.lua
-- ---------------

-- Functions used in the ingame help windows for formatting the text and pictures.

-- RST
-- .. function:: dependencies_basic(images[, text = nil])
--
--    Creates a dependencies line of any length.
--
--    :arg images: images in the correct order from left to right as table (set in {}).
--    :arg text: comment of the image.
--    :returns: a row of pictures connected by arrows.
--
function dependencies_basic(images, text)
	if not text then
		text = ""
	end

	local string = "image=" .. images[1]
	for k,v in ipairs({table.unpack(images,2)}) do
		string = string .. ";pics/arrow-right.png;" .. v
	end

	return rt(string, text)
end


-- RST
-- .. function:: dependencies(items[, text = nil])
--
--    Creates a dependencies line of any length.
--
--    :arg items: ware, worker and/or building descriptions in the correct order
--                from left to right as table (set in {}).
--    :arg text: comment of the image.
--    :returns: a row of pictures connected by arrows.
--
function dependencies(items, text)
	if not text then
		text = ""
	end
	local string = "image=" .. items[1].icon_name
	for k,v in ipairs({table.unpack(items,2)}) do
		string = string .. ";pics/arrow-right.png;" ..  v.icon_name
	end
	return rt(string, p(text))
end


-- RST
-- .. function:: dependencies_resi(resource, items[, text = nil])
--
--    Creates a dependencies line of any length for resources (that don't have menu.png files).
--
--    :arg resource: name of the geological resource.
--    :arg items: ware/building descriptions in the correct order from left to right as table (set in {}).
--    :arg text: comment of the image.
--    :returns: a row of pictures connected by arrows.
--
function dependencies_resi(resource, items, text)
	if not text then
		text = ""
	end
	string = "image=tribes/immovables/" .. resource  .. "/idle_00.png"
	for k,v in ipairs({table.unpack(items)}) do
		string = string .. ";pics/arrow-right.png;" ..  v.icon_name
	end
	return rt(string, p(text))
end


--  =======================================================
--  *************** Dependencies functions ****************
--  =======================================================

-- RST
-- .. function:: dependencies_training_food
--
--    Creates dependencies lines for food in training sites.
--
--    :arg foods: an array of arrays with food items. Outer array has "and" logic and
--	          will appear from back to front, inner arrays have "or" logic
--    :returns: a list of food descriptions with images
--
function dependencies_training_food(foods)
	local result = ""
	for countlist, foodlist in pairs(foods) do
		local images = ""
		local text = ""
		for countfood, food in pairs(foodlist) do
			local ware_description = wl.Game():get_ware_description(food)
			if(countfood > 1) then
				images = images .. ";"
				text = _"%1$s or %2$s":bformat(text, ware_description.descname)
			else
				text = ware_description.descname
			end
			images = ware_description.icon_name
		end
		if(countlist > 1) then
			text = _"%s and":bformat(text)
		end
		result = image_line(images, 1, p(text)) .. result
	end
	return result
end


-- RST
-- .. function:: dependencies_training_weapons(weapons)
--
--    Creates a dependencies line for any number of weapons.
--
--    :arg weapons: an array of weapon names
--    :returns: a list weapons images with the producing and receiving building
--
function dependencies_training_weapons(weapons)
	local result;
	local producers;
	for count, weaponname in pairs(weapons) do
		local weapon_description = wl.Game():get_ware_description(weaponname)
		for i, producer in ipairs(weapon_description.producers) do
			if (producers[producer] == nil) then
				producers[producer] = {}
			end
			producers[producer][weaponname] = true;
		end
	end

	local building_count = 0;
	for manufacturer, weaponnames in ipairs(producers) do
		local manufacturer_description = wl.Game():get_building_description(manufacturer)
		local weaponsstring = ""
		for count, weapon in pairs(weaponnames) do
			if(count > 1) then
				weaponsstring = weaponsstring .. ";"
			end
			local weapon_description = wl.Game():get_ware_description(weapon)
			weaponsstring = weaponsstring .. weapon_description.icon_name
		end
		local equipmentstring
		if (building_count == 0) then
			-- TRANSLATORS: This is a headline, you can see it in the building help for trainingsites, in the dependencies section
			equipmentstring = _"and equipment from"
		else
			-- TRANSLATORS: This is a headline, you can see it in the building help for trainingsites, in the dependencies section
			equipmentstring = _"or equipment from"
		end
		building_count = building_count + 1;
		result = result .. rt(p(equipmentstring)) ..
			dependencies_basic(
				{manufacturer_description.icon_name, weaponsstring},
				rt(p(manufacturer_description.descname))
		)
	end
	return result
end


--  =======================================================
--  ************* Main buildinghelp functions *************
--  =======================================================


-- RST
-- .. function building_help_general_string(building_description, resourcename, purpose[, note])
--
--    Creates the string for the general section in building help
--
--    :arg building_description: The building's building description from C++
--    :returns: rt of the formatted text
--
function building_help_general_string(building_description)
	-- Need to get the building description again to make sure we have the correct type, e.g. "productionsite"
	local building_description = wl.Game():get_building_description(building_description.name)

	local helptexts = building_description.helptexts
	local result = rt(h2(_"Lore")) ..
		rt("image=" .. building_description.representative_image, p(helptexts["lore"]))
	if (helptexts["lore_author"] ~= "") then
		result = result .. rt("text-align=right",
		                      p("font-size=10 font-style=italic",
		                        helptexts["lore_author"]))
	end

	result = result .. rt(h2(_"General"))
	result = result .. rt(h3(_"Purpose:"))

-- TODO(GunChleoc) "carrier" for headquarters, "ship" for ports, "scout" for scouts_hut, "shipwright" for shipyard?
-- TODO(GunChleoc) use aihints for gamekeeper, forester?
	local representative_resource = nil
	if (building_description.type_name == "productionsite" or
	    building_description.type_name == "militarysite"
	    building_description.type_name == "trainingsite" or) then
		representative_resource = building_description.output_ware_types[1]
		if(not representative_resource) then
			representative_resource = building_description.output_worker_types[1]
		end
-- TODO(GunChleoc) need a bob_descr for the ship -> port and shipyard
-- TODO(GunChleoc) create descr objects for flag, portdock, ...
	elseif (building_description.is_port or building_description.name == "shipyard") then
		representative_resource = nil
	elseif (building_description.type_name == "warehouse") then
		representative_resource = wl.Game():get_ware_description("log")
	end

	if(representative_resource) then
		result = result .. image_line(representative_resource.icon_name, 1, p(helptexts.purpose))
	else
		result = result .. rt(p(helptexts.purpose))
	end

	if (helptexts.note ~= "") then
		result = result .. rt(h3(_"Note:")) .. rt(p(helptexts.note))
	end

	if(building_description.type_name == "productionsite") then
		if(building_description.workarea_radius and building_description.workarea_radius > 0) then
			result = result .. text_line(_"Working radius:", building_description.workarea_radius)
		end

	elseif(building_description.type_name == "warehouse") then
		result = result .. rt(h3(_"Healing:")
			.. p(_"Garrisoned soldiers heal %s per second":bformat(building_description.heal_per_second)))
		result = result .. text_line(_"Conquer range:", building_description.conquers)

	elseif(building_description.type_name == "militarysite") then
		result = result .. rt(h3(_"Healing:")
			.. p(_"Garrisoned soldiers heal %s per second":bformat(building_description.heal_per_second)))
		result = result .. text_line(_"Capacity:", building_description.max_number_of_soldiers)
		result = result .. text_line(_"Conquer range:", building_description.conquers)

	elseif(building_description.type_name == "trainingsite") then
		result = result .. rt(h3(_"Training:"))
		if(building_description.max_attack and building_description.min_attack) then
			-- TRANSLATORS: %1$s = Health, Evade, Attack or Defense. %2$s and %3$s are numbers.
			result = result .. rt(p(_"Trains ‘%1$s’ from %2$s up to %3$s":
				bformat(_"Attack", building_description.min_attack, building_description.max_attack+1)))
		end
		if(building_description.max_defense and building_description.min_defense) then
			result = result .. rt(p( _"Trains ‘%1$s’ from %2$s up to %3$s":
				bformat(_"Defense", building_description.min_defense, building_description.max_defense+1)))
		end
		if(building_description.max_evade and building_description.min_evade) then
			result = result .. rt(p( _"Trains ‘%1$s’ from %2$s up to %3$s":
				bformat(_"Evade", building_description.min_evade, building_description.max_evade+1)))
		end
		if(building_description.max_hp and building_description.min_hp) then
			result = result .. rt(p(_"Trains ‘%1$s’ from %2$s up to %3$s":
				bformat(_"Health", building_description.min_hp, building_description.max_hp+1)))
		end
		result = result .. text_line(_"Capacity:", building_description.max_number_of_soldiers)
	end
	result = result .. text_line(_"Vision range:", building_description.vision_range)
	return result
end


-- RST
-- .. function:: building_help_dependencies_production(tribename, building_description)
--
--    The input and output wares of a productionsite
--
--    :arg tribename: e.g. "barbarians".
--    :arg building_description: The building description we get from C++
--    :returns: an rt string with images describing a chain of ware/building dependencies
--
function building_help_dependencies_production(tribename, building_description)
	local building_description = wl.Game():get_building_description(building_description.name)
	local result = ""
	local hasinput = false
	for i, ware_description in ipairs(building_description.inputs) do
	 hasinput = true
		for j, producer in ipairs(ware_description.producers) do
			result = result .. dependencies(
				{producer, ware_description},
				_"%1$s from: %2$s":bformat(ware_description.descname, producer.descname)
			)
		end
	end
	if (hasinput) then
		result =  rt(h3(_"Incoming:")) .. result
	end

	if ((not hasinput) and building_description.output_ware_types[1]) then
		result = result .. rt(h3(_"Collects:"))
		for i, ware_description in ipairs(building_description.output_ware_types) do
			result = result ..
				dependencies({building_description, ware_description}, ware_description.descname)
		end
		for i, worker_description in ipairs(building_description.output_worker_types) do
			result = result ..
				dependencies({building_description, worker_description}, worker_description.descname)
		end

	elseif (building_description.is_mine) then
		-- TRANSLATORS: This is a verb (The miner mines)
		result = result .. rt(h3(_"Mines:"))
		for i, ware_description in ipairs(building_description.output_ware_types) do

			-- Need to hack this, because resource != produced ware.
			local resi_name = ware_description.name
			if(resi_name == "iron_ore") then resi_name = "iron"
			elseif(resi_name == "granite") then resi_name = "stones"
			elseif(resi_name == "diamond") then resi_name = "stones"
			elseif(resi_name == "quartz") then resi_name = "stones"
			elseif(resi_name == "marble") then resi_name = "stones"
			elseif(resi_name == "gold_ore") then resi_name = "gold" end
			result = result .. dependencies_resi(
				"resi_"..resi_name.."2",
				{building_description, ware_description},
				ware_description.descname
			)
		end

	else
		if(building_description.output_ware_types[1] or building_description.output_worker_types[1]) then
			result = result .. rt(h3(_"Produces:"))
		end
		for i, ware_description in ipairs(building_description.output_ware_types) do
			result = result ..
				dependencies({building_description, ware_description}, ware_description.descname)
		end
		for i, worker_description in ipairs(building_description.output_worker_types) do
			result = result ..
				dependencies({building_description, worker_description}, worker_description.descname)
		end
	end

	local outgoing = ""
	for i, ware_description in ipairs(building_description.output_ware_types) do
		-- constructionsite isn't listed with the consumers, so we need a special check
		if (ware_description.is_construction_material(tribename)) then
			local constructionsite_description =
			   wl.Game():get_building_description("constructionsite")
			outgoing = outgoing .. dependencies({ware_description, constructionsite_description},
															 constructionsite_description.descname)
		end

		for j, consumer in ipairs(ware_description.consumers) do
			outgoing = outgoing .. dependencies({ware_description, consumer}, consumer.descname)
		end

		-- soldiers aren't listed with the consumers
		local soldier
		-- NOCOM(GunChleoc): This is now atlantean_soldier etc. Ugly hack, can this be improved?
		if (tribename == "atlanteans") then
		   soldier = wl.Game():get_worker_description("atlanteans_soldier")
		elseif (tribename == "barbarians") then
		   soldier = wl.Game():get_worker_description("barbarians_soldier")
		else
		   soldier = wl.Game():get_worker_description("empire_soldier")
		end
		local addsoldier = false
		for j, buildcost in ipairs(soldier.buildcost) do
			if(buildcost == ware) then
			local headquarters_description
			-- NOCOM(GunChleoc): This is now atlantean_headquarters etc. Ugly hack, can this be improved?
			if (tribename == "atlanteans") then
				headquarters_description = wl.Game():get_worker_description("atlanteans_headquarters")
			elseif (tribename == "barbarians") then
				headquarters_description = wl.Game():get_worker_description("barbarians_headquarters")
			else
				headquarters_description = wl.Game():get_worker_description("empire_headquarters")
			end
			outgoing = outgoing .. dependencies({ware, headquarters_description, soldier}, soldier.descname)
			end
		end
	end
	if (outgoing ~= "") then result = result .. rt(h3(_"Outgoing:")) .. outgoing end

	if (result == "") then result = rt(p(_"None")) end
	return rt(h2(_"Dependencies")) .. result
end

-- RST
-- .. function:: building_help_dependencies_training(tribename, building_description)
--
--    Shows the production dependencies for a training site.
--
--    :arg tribename: name of the tribe.
--    :arg building_description: the trainingsite's building description from C++
--    :returns: rt string with training dependencies information.
--
function building_help_dependencies_training(tribename, building_description)
	local result = rt(h2(_"Dependencies"))
	if(building_description.max_hp and building_description.min_hp) then
		result = result .. rt(h3(_"Health Training:")) ..
		result = result .. rt(h3(_"Soldiers:")) ..
		result = result ..
			dependencies_basic({
				"tribes/workers/" .. tribename .. "/soldier/hp_level" .. building_description.min_hp .. ".png",
				building_description.icon_name,
				"tribes/workers/" .. tribename .. "/soldier/hp_level" .. (building_description.max_hp + 1) ..".png"})
		result = result .. dependencies_training_food(building_description.food_hp)
		result = result .. dependencies_training_weapons(building_description.weapons_hp)
	end
	if(building_description.max_attack and building_description.min_attack) then
		result = result .. rt(h3(_"Attack Training:")) ..
		result = result .. rt(h3(_"Soldiers:")) ..
			dependencies_basic({
				"tribes/workers/" .. tribename .. "/soldier/attack_level" .. building_description.min_attack .. ".png",
				building_description.icon_name,
				"tribes/workers/" .. tribename .. "/soldier/attack_level" .. (building_description.max_attack + 1) ..".png"})
		result = result .. dependencies_training_food(building_description.food_attack)
		result = result .. dependencies_training_weapons(building_description.weapons_attack)
	end
	if(building_description.max_defense and building_description.min_defense) then
		result = result .. rt(h3(_"Defense Training:")) ..
		result = result .. rt(h3(_"Soldiers:")) ..
		result = result ..
			dependencies_basic({
				"tribes/workers/" .. tribename .. "/soldier/defense_level" .. building_description.min_defense .. ".png",
				building_description.icon_name,
				"tribes/workers/" .. tribename .. "/soldier/defense_level" .. (building_description.max_defense + 1) ..".png"})
		result = result .. dependencies_training_food(building_description.food_defense)
		result = result .. dependencies_training_weapons(building_description.weapons_defense)
	end
	if(building_description.max_evade and building_description.min_evade) then
		result = result .. rt(h3(_"Evade Training:")) ..
		result = result .. rt(h3(_"Soldiers:")) ..
		result = result ..
			dependencies_basic({
				"tribes/workers/" .. tribename .. "/soldier/evade_level" .. building_description.min_evade .. ".png",
				building_description.icon_name,
				"tribes/workers/" .. tribename .. "/soldier/evade_level" .. (building_description.max_evade + 1) ..".png"})
		result = result .. dependencies_training_food(building_description.food_evade)
		result = result .. dependencies_training_weapons(building_description.weapons_evade)
	end
	return result
end


-- Helper function for building_help_building_section
function building_help_building_line(ware_description, amount)
	amount = tonumber(amount)
	local image = ware_description.icon_name
	local result = ""
	local imgperline = 6
	local temp_amount = amount

	while (temp_amount > imgperline) do
		result = result .. image_line(image, imgperline)
		temp_amount = temp_amount - imgperline
	end
	-- TRANSLATORS: %1$d is a number, %2$s the name of a ware, e.g. 12x Stone
	result = image_line(image, temp_amount, p(_"%1$dx %2$s":bformat(amount, ware_description.descname))) .. result
	return result

end

-- RST
--
-- .. function:: building_help_building_section(building_description)
--
--    Formats the "Building" section in the building help: Enhancing info, costs and space required
--
--    :arg building_description: The building description we get from C++
--    :returns: an rt string describing the building section
--
function building_help_building_section(building_description)

	local result = rt(h2(_"Building"))

	-- Space required
	if (building_description.is_mine) then
		result = result .. text_line(_"Space required:",_"Mine plot","pics/mine.png")
	elseif (building_description.is_port) then
		result = result .. text_line(_"Space required:",_"Port plot","pics/port.png")
	else
		if (building_description.size == 1) then
			result = result .. text_line(_"Space required:",_"Small plot","pics/small.png")
		elseif (building_description.size == 2) then
			result = result .. text_line(_"Space required:",_"Medium plot","pics/medium.png")
		elseif (building_description.size == 3) then
			result = result .. text_line(_"Space required:",_"Big plot","pics/big.png")
		else
			result = result .. p(_"Space required:" .. _"Unknown")
		end
	end

	-- Enhanced from
	if (building_description.buildable or building_description.enhanced) then

		if (building_description.buildable and building_description.enhanced) then
			result = result .. text_line(_"Note:",
				_"This building can either be built directly or obtained by enhancing another building.")
		end

		if (building_description.buildable) then
			-- Build cost
			if (building_description.buildable and building_description.enhanced) then
				result = result .. rt(h3(_"Direct build cost:"))
			else
				result = result .. rt(h3(_"Build cost:"))
			end
			for ware, amount in pairs(building_description.build_cost) do
				local ware_description = wl.Game():get_ware_description(ware)
				result = result .. building_help_building_line(ware_description, amount)
			end
		end
		local former_building = nil
		if (building_description.enhanced) then
			former_building = building_description.get_enhanced_from
				if (building_description.buildable) then
					result = result .. text_line(_"Or enhanced from:", former_building.descname)
				else
					result = result .. text_line(_"Enhanced from:", former_building.descname)
				end

			for ware, amount in pairs(building_description.enhancement_cost) do
				local ware_description = wl.Game():get_ware_description(ware)
				result = result .. building_help_building_line(ware_description, amount)
			end

			-- Cumulative cost
			result = result .. rt(h3(_"Cumulative cost:"))
			local warescost = {}
			for ware, amount in pairs(building_description.enhancement_cost) do
				if (warescost[ware]) then
					warescost[ware] = warescost[ware] + amount
				else
					warescost[ware] = amount
				end
			end

			local former_buildings = {};
			former_building = building_description
			while former_building.enhanced do
				former_building = former_building.get_enhanced_from
				table.insert(former_buildings, former_building)
			end

			for index, former in pairs(former_buildings) do
				former_building = wl.Game():get_building_description(former)
				if (former_building.buildable) then
					for ware, amount in pairs(former_building.build_cost) do
						if (warescost[ware]) then
							warescost[ware] = warescost[ware] + amount
						else
							warescost[ware] = amount
						end
					end
				elseif (former_building.enhanced) then
					for ware, amount in pairs(former_building.enhancement_cost) do
						if (warescost[ware]) then
							warescost[ware] = warescost[ware] + amount
						else
							warescost[ware] = amount
						end
					end
				end
			end
			if (warescost ~= {}) then
				for ware, amount in pairs(warescost) do
					local ware_description = wl.Game():get_ware_description(ware)
					result = result .. building_help_building_line(ware_description, amount)
				end
			else
				result = result .. rt(p(_"Unknown"))
			end

			-- Dismantle yields
			if (building_description.buildable) then
				result = result .. rt(h3(_"If built directly, dismantle yields:"))
				for ware, amount in pairs(building_description.returned_wares) do
					local ware_description = wl.Game():get_ware_description(ware)
					result = result .. building_help_building_line(ware_description, amount)
				end
				result = result .. rt(h3(_"If enhanced, dismantle yields:"))
			else
				result = result .. rt(h3(_"Dismantle yields:"))
			end
			local warescost = {}
			for ware, amount in pairs(building_description.returned_wares_enhanced) do
				if (warescost[ware]) then
					warescost[ware] = warescost[ware] + amount
				else
					warescost[ware] = amount
				end
			end
			for index, former in pairs(former_buildings) do
				former_building = wl.Game():get_building_description(former)
				if (former_building.buildable) then
					for ware, amount in pairs(former_building.returned_wares) do
						if (warescost[ware]) then
							warescost[ware] = warescost[ware] + amount
						else
							warescost[ware] = amount
						end
					end
				elseif (former_building.enhanced) then
					for ware, amount in pairs(former_building.returned_wares_enhanced) do
						if (warescost[ware]) then
							warescost[ware] = warescost[ware] + amount
						else
							warescost[ware] = amount
						end
					end
				end
			end
			if (warescost ~= {}) then
				for ware, amount in pairs(warescost) do
					local ware_description = wl.Game():get_ware_description(ware)
					result = result .. building_help_building_line(ware_description, amount)
				end
			else
				result = result .. rt(p(_"Unknown"))
			end
		-- Buildable
		else
			-- Dismantle yields
			result = result .. rt(h3(_"Dismantle yields:"))
			for ware, amount in pairs(building_description.returned_wares) do
				local ware_description = wl.Game():get_ware_description(ware)
				result = result .. building_help_building_line(ware_description, amount)
			end
		end

		-- Can be enhanced to
		if (building_description.enhancement) then
			result = result .. text_line(_"Can be enhanced to:", building_description.enhancement.descname)
			for ware, amount in pairs(building_description.enhancement.enhancement_cost) do
				local ware_description = wl.Game():get_ware_description(ware)
				result = result .. building_help_building_line(ware_description, amount)
			end
		end
	end
	return result
end



-- RST
-- .. function building_help_crew_string(building_description)
--
--    Displays the building's workers with an image and the tool they use
--
--    :arg building_description: the building_description from C++.
--    :returns: Workers/Crew section of the help file
--
function building_help_crew_string(building_description)
	-- Need to get the building description again to make sure we have the correct type, e.g. "productionsite"
	local building_description = wl.Game():get_building_description(building_description.name)
	local result = ""

	if(building_description.type_name == "productionsite" or building_description.type_name == "trainingsite") then

		result = result .. rt(h2(_"Workers")) .. rt(h3(_"Crew required:"))

		local worker_description = building_description.working_positions[1]
		local becomes_description = nil
		local number_of_workers = 0
		local toolnames = {}

		for i, worker_description in ipairs(building_description.working_positions) do

			-- Get the tools for the workers.
			if(worker_description.buildable) then
				for j, buildcost in ipairs(worker_description.buildcost) do
					-- NOCOM(GunChleoc): Different carrier types now
					if( not (buildcost == "carrier" or buildcost == "none" or buildcost == nil)) then
						toolnames[#toolnames + 1] = buildcost
					end
				end
			end

			becomes_description = worker_description.becomes
			number_of_workers = number_of_workers + 1

			if(becomes_description) then
				result = result .. image_line(worker_description.icon_name, 1,
					p(_"%s or better":bformat(worker_description.descname)))
			else
				result = result .. image_line(worker_description.icon_name, 1,
					p(worker_description.descname))
			end
		end

		if(#toolnames > 0) then
			result = result .. building_help_tool_string(toolnames, number_of_workers)
		end

		if(becomes_description) then

			result = result .. rt(h3(_"Experience levels:"))
			local exp_string = _"%s to %s (%s EP)":format(
					worker_description.descname,
					becomes_description.descname,
					worker_description.needed_experience
				)

			worker_description = becomes_description
			becomes_description = worker_description.becomes
			if(becomes_description) then
				exp_string = exp_string .. "<br>" .. _"%s to %s (%s EP)":format(
						worker_description.descname,
						becomes_description.descname,
						worker_description.needed_experience
					)
			end
			result = result ..  rt("text-align=right", p(exp_string))
		end
	end

	return result
end


-- RST
-- .. function building_help_tool_string( toolname)
--
--    Displays tools with an intro text and images
--
--    :arg toolnames: e.g. {"shovel", "basket"}.
--    :arg no_of_workers: the number of workers using the tools; for plural formatting.
--    :returns: text_line for the tools
--
function building_help_tool_string(toolnames, no_of_workers)
	local result = rt(h3(ngettext("Worker uses:","Workers use:", no_of_workers)))
	local game  = wl.Game();
	for i, toolname in ipairs(toolnames) do
		local ware_description = game:get_ware_description(toolname)
		result = result .. image_line(ware_description.icon_name, 1, p(ware_description.descname))
	end
	return result
end

-- RST
-- .. building_help_production_section(building_description)
--
--    Displays the production/performance section with a headline
--
--    :arg building_description: The building's building description from C++
--    :returns: rt for the production section
--
function building_help_production_section(building_description)
	if (building_description.helptexts.performance ~= "") then
		return rt(h2(_"Production")) ..
		  text_line(_"Performance:", building_description.helptexts.performance)
	else
		return ""
	end
end


-- RST
-- .. function building_help(building_description, tribename)
--
--    Main function to create a building help string.
--
--    :arg building_description: The building's building description from C++
--    :arg tribename: The name of the tribe that is used for tribe-specific information.
--    :returns: rt of the formatted text
--
function building_help(building_description, tribename)
	-- Need to get the building description again to make sure we have the correct type, e.g. "productionsite"
	local building_description = wl.Game():get_building_description(building_description.name)
	local result = ""
	if (building_description.type_name == "productionsite") then
		return building_help_general_string(building_description) ..
			building_help_dependencies_production(tribename, building_description) ..
			building_help_crew_string(building_description) ..
			building_help_building_section(building_description) ..building_help_production_section()
	else if (building_description.type_name == "militarysite") then
		return building_help_general_string(building_description) ..
			building_help_building_section(building_description)
	else if (building_description.type_name == "warehouse") then
		if (building_description.is_port) then
			return building_help_general_string(building_description) ..
				-- TODO(GunChleoc) expedition costs here?
				building_help_building_section(building_description) ..
				building_help_production_section()
		else
			return building_help_general_string(building_description) ..
				building_help_building_section(building_description)
		end
	else if (building_description.type_name == "trainingsite") then
		return building_help_general_string(building_description) ..
			building_help_dependencies_training(tribename, building_description) ..
			building_help_crew_string(building_description) ..
			building_help_building_section(building_description) ..building_help_production_section()
	else if (building_description.type_name == "constructionsite" or
				building_description.type_name == "dismantlesite") then
				-- TODO(GunChleoc) Get them a crew string for the builder
		return building_help_general_string(building_description)
	else
		return ""
	end
}

-- The main function call
return {
   func = function(building_description, tribename)
		return building_help(building_description, tribename)
   end
}
