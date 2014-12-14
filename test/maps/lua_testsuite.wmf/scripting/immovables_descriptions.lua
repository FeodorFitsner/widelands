set_textdomain("tribe_barbarians")

test_descr = lunit.TestCase("Immovable descriptions test")
function test_descr:test_instantiation_forbidden()
   assert_error("Cannot instantiate", function()
      wl.map.BuildingDescription()
   end)
   assert_error("Cannot instantiate", function()
      wl.map.WareDescription()
   end)
   assert_error("Cannot instantiate", function()
      wl.map.WorkerDescription()
   end)
end

--  =======================================================
--  ***************** BuildingDescription *****************
--  =======================================================

function test_descr:test_building_descr()
   assert_error("Wrong tribe", function() egbase:get_building_description("XXX","barbarians_sentry") end)
   assert_error("Wrong building", function() egbase:get_building_description("barbarians","XXX") end)
   assert_error("Wrong number of parameters: 1", function() egbase:get_building_description("XXX") end)
   assert_error("Wrong number of parameters: 3", function() egbase:get_building_description("XXX","YYY","ZZZ") end)
end

-- NOCOM(#GunChleoc): We won't need the tribe parameter in the future.
-- This is actually a property of MapOjectDescription
function test_descr:test_descname()
   assert_equal(_"Lumberjack’s Hut", egbase:get_building_description("barbarians","barbarians_lumberjacks_hut").descname)
   assert_equal(_"Battle Arena", egbase:get_building_description("barbarians","barbarians_battlearena").descname)
   assert_equal(_"Fortress", egbase:get_building_description("barbarians","barbarians_fortress").descname)
   assert_equal(_"Coal Mine", egbase:get_building_description("barbarians","barbarians_coalmine").descname)
end

-- This is actually a property of MapOjectDescription
function test_descr:test_name()
   assert_equal("barbarians_lumberjacks_hut", egbase:get_building_description("barbarians","barbarians_lumberjacks_hut").name)
   assert_equal("barbarians_battlearena", egbase:get_building_description("barbarians","barbarians_battlearena").name)
   assert_equal("barbarians_fortress", egbase:get_building_description("barbarians","barbarians_fortress").name)
   assert_equal("barbarians_coalmine", egbase:get_building_description("barbarians","barbarians_coalmine").name)
end

function test_descr:test_build_cost()
   local total_cost = function(t)
      local cost = 0
      for name, count in pairs(t) do
         cost = cost + count
      end
      return cost
   end
   assert_equal(2, total_cost(egbase:get_building_description("barbarians","barbarians_sentry").build_cost))
   assert_equal(20, total_cost(egbase:get_building_description("barbarians","barbarians_fortress").build_cost))
   assert_equal(0, total_cost(egbase:get_building_description("barbarians","barbarians_citadel").build_cost))
end

function test_descr:test_buildable()
   assert_equal(false, egbase:get_building_description("barbarians","barbarians_headquarters").buildable)
   assert_equal(true, egbase:get_building_description("barbarians","barbarians_sentry").buildable)
end

function test_descr:test_conquers()
   assert_equal(0, egbase:get_building_description("barbarians","barbarians_lumberjacks_hut").conquers)
   assert_equal(6, egbase:get_building_description("barbarians","barbarians_sentry").conquers)
   assert_equal(9, egbase:get_building_description("barbarians","barbarians_headquarters").conquers)
   assert_equal(0, egbase:get_building_description("barbarians","vcoalmine").conquers)
end

function test_descr:test_destructible()
   assert_equal(false, egbase:get_building_description("barbarians","barbarians_headquarters").destructible)
   assert_equal(true, egbase:get_building_description("barbarians","barbarians_sentry").destructible)
end

function test_descr:test_enhanced()
   assert_equal(false, egbase:get_building_description("barbarians","barbarians_headquarters").enhanced)
   assert_equal(true, egbase:get_building_description("barbarians","barbarians_axfactory").enhanced)
end

function test_descr:test_enhancement_cost()
   local total_cost = function(t)
      local cost = 0
      for name, count in pairs(t) do
         cost = cost + count
      end
      return cost
   end
   assert_equal(0, total_cost(egbase:get_building_description("barbarians","barbarians_sentry").enhancement_cost))
   assert_equal(20, total_cost(egbase:get_building_description("barbarians","barbarians_citadel").enhancement_cost))
end

function test_descr:test_enhancement()
	assert_equal("inn", egbase:get_building_description("barbarians","barbarians_tavern").enhancement.name)
	assert_equal(nil, egbase:get_building_description("barbarians","barbarians_lumberjacks_hut").enhancement)
end

function test_descr:test_icon_name()
   assert_equal("tribes/buildings/warehouses/barbarians/headquarters/menu.png", egbase:get_building_description("barbarians","barbarians_headquarters").icon_name)
end

function test_descr:test_ismine()
   assert_equal(false, egbase:get_building_description("barbarians","barbarians_headquarters").is_mine)
   assert_equal(true, egbase:get_building_description("barbarians","barbarians_ironmine").is_mine)
end

function test_descr:test_isport()
   assert_equal(false, egbase:get_building_description("barbarians","barbarians_headquarters").is_port)
   assert_equal(true, egbase:get_building_description("barbarians","barbarians_port").is_port)
end

function test_descr:test_returned_wares()
   local total_cost = function(t)
      local cost = 0
      for name, count in pairs(t) do
         cost = cost + count
      end
      return cost
   end
   assert_equal(1, total_cost(egbase:get_building_description("barbarians","barbarians_sentry").returned_wares))
   assert_equal(9, total_cost(egbase:get_building_description("barbarians","barbarians_fortress").returned_wares))
   assert_equal(0, total_cost(egbase:get_building_description("barbarians","barbarians_citadel").returned_wares))
end

function test_descr:test_returned_wares_enhanced()
   local total_cost = function(t)
      local cost = 0
      for name, count in pairs(t) do
         cost = cost + count
      end
      return cost
   end
   assert_equal(0, total_cost(egbase:get_building_description("barbarians","barbarians_sentry").returned_wares_enhanced))
   assert_equal(0, total_cost(egbase:get_building_description("barbarians","barbarians_fortress").returned_wares_enhanced))
   assert_equal(10, total_cost(egbase:get_building_description("barbarians","vcitadel").returned_wares_enhanced))
end

function test_descr:test_size()
   assert_equal(1, egbase:get_building_description("barbarians","barbarians_lumberjacks_hut").size)
   assert_equal(2, egbase:get_building_description("barbarians","barbarians_reed_yard").size)
   assert_equal(3, egbase:get_building_description("barbarians","barbarians_fortress").size)
   assert_equal(1, egbase:get_building_description("barbarians","barbarians_coalmine").size)
end

function test_descr:test_type()
   assert_equal("militarysite", egbase:get_building_description("barbarians","barbarians_sentry").descr.type_name)
end

function test_descr:test_vision_range()
-- if vision_range is not set in the conf, it is get_conquers() + 4
   assert_equal(4, egbase:get_building_description("barbarians","barbarians_lumberjacks_hut").vision_range)
   assert_equal(2, egbase:get_building_description("barbarians","constructionsite").vision_range)
   assert_equal(4+11, egbase:get_building_description("barbarians","barbarians_fortress").vision_range)
   assert_equal(17, egbase:get_building_description("barbarians","barbarians_tower").vision_range)
end

--  =======================================================
--  ************** ProductionSiteDescription **************
--  =======================================================

-- This is actually a property of MapOjectDescription
function test_descr:test_descname()
   assert_equal(_"Coal Mine", egbase:get_building_description("barbarians","barbarians_coalmine").descname)
end

-- This is actually a property of MapOjectDescription
function test_descr:test_name()
   assert_equal("barbarians_coalmine", egbase:get_building_description("barbarians","barbarians_coalmine").name)
end

function test_descr:test_inputs()
	local building_description = egbase:get_building_description("barbarians","barbarians_bakery")
	assert_equal("wheat", building_description.inputs[1].name)
	assert_equal("water", building_description.inputs[2].name)
	building_description = egbase:get_building_description("barbarians","barbarians_lumberjacks_hut")
	assert_equal(nil, building_description.inputs[1])
end

function test_descr:test_output_ware_types()
	local building_description = egbase:get_building_description("barbarians","barbarians_bakery")
	assert_equal("bread_barbarians", building_description.output_ware_types[1].name)
	building_description = egbase:get_building_description("barbarians","barbarians_gamekeepers_hut")
	assert_equal(nil, building_description.output_ware_types[1])
end

function test_descr:test_output_worker_types()
	local building_description = egbase:get_building_description("barbarians","barbarians_cattlefarm")
	assert_equal("barbarians_ox", building_description.output_worker_types[1].name)
	building_description = egbase:get_building_description("barbarians","barbarians_gamekeepers_hut")
	assert_equal(nil, building_description.output_ware_types[1])
end

function test_descr:test_type()
   assert_equal("productionsite", egbase:get_building_description("barbarians","barbarians_coalmine").descr.type_name)
end

function test_descr:test_working_positions()
	local building_description = egbase:get_building_description("barbarians","barbarians_deeper_coalmine")
	assert_equal("barbarians_miner_master", building_description.working_positions[1].name)
	assert_equal("barbarians_miner_chief", building_description.working_positions[2].name)
	assert_equal("barbarians_miner", building_description.working_positions[3].name)
	building_description = egbase:get_building_description("barbarians","barbarians_big_inn")
	assert_equal("barbarians_innkeeper", building_description.working_positions[1].name)
	assert_equal("barbarians_innkeeper", building_description.working_positions[2].name)
end


--  =======================================================
--  *************** MilitarySiteDescription ***************
--  =======================================================

-- This is actually a property of MapOjectDescription
function test_descr:test_descname()
   assert_equal(_"Sentry", egbase:get_building_description("barbarians","barbarians_sentry").descname)
end

-- This is actually a property of MapOjectDescription
function test_descr:test_name()
   assert_equal("barbarians_sentry", egbase:get_building_description("barbarians","barbarians_sentry").name)
end

function test_descr:test_heal_per_second()
   assert_equal(80, egbase:get_building_description("barbarians","barbarians_sentry").heal_per_second)
   assert_equal(170, egbase:get_building_description("barbarians","barbarians_fortress").heal_per_second)
end

function test_descr:test_max_number_of_soldiers()
   assert_equal(2, egbase:get_building_description("barbarians","barbarians_sentry").max_number_of_soldiers)
   assert_equal(8, egbase:get_building_description("barbarians","barbarians_fortress").max_number_of_soldiers)
end

function test_descr:test_type()
   assert_equal("militarysite", egbase:get_building_description("barbarians","barbarians_sentry").descr.type_name)
end


--  =======================================================
--  *************** TrainingSiteDescription ***************
--  =======================================================

-- This is actually a property of MapOjectDescription
function test_descr:test_descname()
   assert_equal(_"Battle Arena", egbase:get_building_description("barbarians","barbarians_battlearena").descname)
end

-- This is actually a property of MapOjectDescription
function test_descr:test_name()
   assert_equal("battlearena", egbase:get_building_description("barbarians","barbarians_battlearena").name)
end

function test_descr:test_max_attack()
   assert_equal(nil, egbase:get_building_description("barbarians","barbarians_battlearena").max_attack)
   assert_equal(4, egbase:get_building_description("barbarians","barbarians_trainingcamp").max_attack)
end

function test_descr:test_max_defense()
   assert_equal(nil, egbase:get_building_description("barbarians","barbarians_battlearena").max_defense)
   assert_equal(nil, egbase:get_building_description("barbarians","barbarians_trainingcamp").max_defense)
   assert_equal(1, egbase:get_building_description("atlanteans","atlanteans_labyrinth").max_defense)
end

function test_descr:test_max_evade()
   assert_equal(1, egbase:get_building_description("barbarians","barbarians_battlearena").max_evade)
   assert_equal(nil, egbase:get_building_description("barbarians","barbarians_trainingcamp").max_evade)
end

function test_descr:test_max_hp()
   assert_equal(nil, egbase:get_building_description("barbarians","barbarians_battlearena").max_hp)
   assert_equal(2, egbase:get_building_description("barbarians","barbarians_trainingcamp").max_hp)
end

function test_descr:test_min_attack()
   assert_equal(nil, egbase:get_building_description("barbarians","barbarians_battlearena").min_attack)
   assert_equal(0, egbase:get_building_description("barbarians","barbarians_trainingcamp").min_attack)
end

function test_descr:test_min_defense()
   assert_equal(nil, egbase:get_building_description("barbarians","barbarians_battlearena").min_defense)
   assert_equal(nil, egbase:get_building_description("barbarians","barbarians_trainingcamp").min_defense)
   assert_equal(0, egbase:get_building_description("atlanteans","atlanteans_labyrinth").min_defense)
end

function test_descr:test_min_evade()
   assert_equal(0, egbase:get_building_description("barbarians","barbarians_battlearena").min_evade)
   assert_equal(nil, egbase:get_building_description("barbarians","barbarians_trainingcamp").min_evade)
end

function test_descr:test_min_hp()
   assert_equal(nil, egbase:get_building_description("barbarians","barbarians_battlearena").min_hp)
   assert_equal(0, egbase:get_building_description("barbarians","barbarians_trainingcamp").min_hp)
end

function test_descr:test_type()
   assert_equal("trainingsite", egbase:get_building_description("barbarians","barbarians_battlearena").descr.type_name)
end


--  =======================================================
--  **************** WarehouseDescription *****************
--  =======================================================

-- This is actually a property of MapOjectDescription
function test_descr:test_descname()
   assert_equal(_"Warehouse", egbase:get_building_description("barbarians","barbarians_warehouse").descname)
end

-- This is actually a property of MapOjectDescription
function test_descr:test_name()
   assert_equal("barbarians_warehouse", egbase:get_building_description("barbarians","barbarians_warehouse").name)
end

function test_descr:test_heal_per_second()
   assert_equal(170, egbase:get_building_description("barbarians","barbarians_warehouse").heal_per_second)
   assert_equal(220, egbase:get_building_description("barbarians","barbarians_headquarters").heal_per_second)
end

function test_descr:test_type()
   assert_equal("warehouse", egbase:get_building_description("barbarians","barbarians_warehouse").type_name)
end


--  =======================================================
--  ***************** WareDescription *****************
--  =======================================================

function test_descr:test_ware_descr()
   assert_error("Wrong tribe", function() egbase:get_ware_description("XXX","thatch_reed") end)
   assert_error("Wrong ware", function() egbase:get_ware_description("barbarians","XXX") end)
   assert_error("Wrong number of parameters: 1", function() egbase:get_ware_description("XXX") end)
   assert_error("Wrong number of parameters: 3", function() egbase:get_ware_description("XXX","YYY","ZZZ") end)
end

-- This is actually a property of MapOjectDescription
function test_descr:test_descname()
   assert_equal(_"Thatch Reed", egbase:get_ware_description("barbarians","thatch_reed").descname)
end

-- This is actually a property of MapOjectDescription
function test_descr:test_name()
   assert_equal("thatch_reed", egbase:get_ware_description("barbarians","thatch_reed").name)
end

function test_descr:test_consumers()
	local ware_description = egbase:get_ware_description("barbarians","coal")
   assert_equal("barbarians_lime_kiln", ware_description.consumers[1].name)
   assert_equal("barbarians_smelting_works", ware_description.consumers[2].name)
   assert_equal("barbarians_warmill", ware_description.consumers[3].name)
   assert_equal("barbarians_axfactory", ware_description.consumers[4].name)
   assert_equal("barbarians_helmsmithy", ware_description.consumers[5].name)
end

function test_descr:test_icon_name()
   assert_equal("tribes/wares/coal/menu.png", egbase:get_ware_description("barbarians","coal").icon_name)
end

function test_descr:test_producers()
	local ware_description = egbase:get_ware_description("barbarians","coal")
	assert_equal("barbarians_charcoal_kiln", ware_description.producers[1].name)
	assert_equal("barbarians_deeper_coalmine", ware_description.producers[2].name)
	assert_equal("barbarians_deep_coalmine", ware_description.producers[3].name)
	assert_equal("barbarians_coalmine", ware_description.producers[4].name)
end

--  =======================================================
--  ****************** WorkerDescription ******************
--  =======================================================

function test_descr:test_worker_descr()
   assert_error("Wrong tribe", function() egbase:get_worker_description("XXX","miner") end)
   assert_error("Wrong worker", function() egbase:get_worker_description("barbarians","XXX") end)
   assert_error("Wrong number of parameters: 1", function() egbase:get_worker_description("XXX") end)
   assert_error("Wrong number of parameters: 3", function() egbase:get_worker_description("XXX","YYY","ZZZ") end)
end

-- This is actually a property of MapOjectDescription
function test_descr:test_descname()
   assert_equal(_"Miner", egbase:get_worker_description("barbarians","barbarians_miner").descname)
end

-- This is actually a property of MapOjectDescription
function test_descr:test_name()
   assert_equal("barbarians_miner", egbase:get_worker_description("barbarians","barbarians_miner").name)
end

function test_descr:test_becomes()
	local worker_descr = egbase:get_worker_description("barbarians","barbarians_miner").becomes
   assert_equal("barbarians_miner_chief", worker_descr.name)
	worker_descr = egbase:get_worker_description("barbarians","barbarians__miner_chief").becomes
   assert_equal("barbarians__miner_master", worker_descr.name)
	worker_descr = egbase:get_worker_description("barbarians","barbarians__miner_master").becomes
   assert_equal(nil, worker_descr)
end

function test_descr:test_icon_name()
   assert_equal("tribes/workers/barbarians/miner/menu.png", egbase:get_worker_description("barbarians","barbarians_miner").icon_name)
end

function test_descr:test_needed_experience()
   assert_equal(19, egbase:get_worker_description("barbarians","barbarians_miner").needed_experience)
   assert_equal(28, egbase:get_worker_description("barbarians","barbarians_miner_chief").needed_experience)
end
