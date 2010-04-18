-- =======================================================================
--                         BaseImmovable Usage Tests                          
-- =======================================================================

-- ====================
-- Creation & Deletion 
-- ====================
-- Note: these next functions implicitly also check that inheritance
-- works because serial is a property of a MapObject.
immovable_creation_tests = lunit.TestCase("Immovable Creation")
function immovable_creation_tests:test_create()
   imm = wl.map.create_immovable("tree1", wl.map.Field(9, 10))
   imm2 = wl.map.create_immovable("tree2", wl.map.Field(10, 10))
   assert_table(imm)
   assert_table(imm2)
   assert_true(imm.serial > 0)
   assert_true(imm2.serial > 0)
   assert_true(imm2.serial > imm.serial)

   imm:remove()
   imm2:remove()
end
function immovable_creation_tests:test_create_tribe_immovables()
   imm = wl.map.create_immovable("field2", wl.map.Field(10,10), "barbarians")
   imm:remove()
end
function immovable_creation_tests:test_create_world_immovables()
   imm = wl.map.create_immovable("tree1", wl.map.Field(10,10), "world")
   imm:remove()
end
function immovable_creation_tests:test_create_world_immovables1()
   imm = wl.map.create_immovable("tree1", wl.map.Field(10,10), nil)
   imm:remove()
end
function immovable_creation_tests:test_create_tribe_immovables_ill_tribe()
   assert_error("Illegal tribe!", function()
      imm = wl.map.create_immovable("field2", wl.map.Field(10,10), "blablub")
   end)
end
function immovable_creation_tests:test_create_tribe_immovables_ill_immovable()
   assert_error("Illegal tribe!", function()
      imm = wl.map.create_immovable("b", wl.map.Field(10,10), "barbarians")
   end)
end
function immovable_creation_tests:test_create_wrong_usage()
   assert_error("Needs table, not integer", function()
      wl.map.create_immovable("tree1", 9, 10) end)
end
function immovable_creation_tests:test_create_wrong_usage()
   assert_error("Needs table, not integer", function()
      wl.map.create_immovable("tree1", 9, 10) end)
end

-- ===================
-- Simple usage tests
-- ===================
immovable_tests = lunit.TestCase("Immovable usage")
function immovable_tests:setup()
   self.i = wl.map.create_immovable("tree1", wl.map.Field(9, 10))
end
function immovable_tests:teardown()
   pcall(self.i.remove, self.i)
end

function immovable_tests:test_wrongusage()
   self.i:remove()
   assert_error("Should not be able to remove an object twice!",
      function() self.i:remove() end
   )
end
function immovable_tests:test_serial_is_readonly()
   assert_error("Serial should be read only", function() self.i.serial = 12 end)
end
function immovable_tests:test_field_access()
   f = wl.map.Field(9,10)
   assert_equal("tree1", f.immovable.name)
end
function immovable_tests:test_map_object_equality()
   f = wl.map.Field(9,10)
   assert_equal(self.i, f.immovable)
end
function immovable_tests:test_field_immovable_nil_when_not_set()
   f = wl.map.Field(10,10)
   assert_equal(nil, f.immovable)
end
function immovable_tests:test_field_immovable_is_read_only()
   f = wl.map.Field(10,10)
   assert_error("f.immovable should be read only!", function()
      f.immovable = self.i
   end)
end

--
-- -- ==============
-- -- Property tests
-- -- ==============
immovable_property_tests = lunit.TestCase("Immovable sizes")
function immovable_property_tests:setup()
   self.none = wl.map.create_immovable("pebble1", wl.map.Field(19, 10))
   self.small = wl.map.create_immovable("tree1", wl.map.Field(18, 10))
   -- TODO: comment this in again
   -- self.medium = wl.game.Player(1):place_building(
   --    "burners_house", wl.map.Field(10,10)
   -- )
   self.big = wl.map.create_immovable("stones4", wl.map.Field(20, 10))
end
function immovable_property_tests:teardown()
   pcall(self.none.remove, self.none)
   pcall(self.small.remove, self.small)
   -- TODO: comment this in again
   -- pcall(self.medium.remove, self.medium)
   -- -- also remove flag
   -- local f = wl.map.Field(10,10)
   -- pcall(f.brn.immovable.remove, f.brn.immovable)
   pcall(self.big.remove, self.big)
end

function immovable_property_tests:test_size_none()
   assert_equal("none", self.none.size)
end
function immovable_property_tests:test_size_small()
   assert_equal("small", self.small.size)
end
-- TODO: comment this in
-- function immovable_property_tests:test_size_medium()
--    assert_equal("medium", self.medium.size)
-- end
function immovable_property_tests:test_size_big()
   assert_equal("big", self.big.size)
end
function immovable_property_tests:test_name_pebble()
   assert_equal("pebble1", self.none.name)
end
function immovable_property_tests:test_name_tree()
   assert_equal("tree1", self.small.name)
end
-- TODO: comment this in
-- function immovable_property_tests:test_name_charcoal_burner()
--    assert_equal("burners_house", self.medium.name)
-- end
function immovable_property_tests:test_name_stone()
   assert_equal("stones4", self.big.name)
end

function immovable_property_tests:test_type_pebble()
   assert_equal("immovable", self.none.type)
end
function immovable_property_tests:test_type_tree()
   assert_equal("immovable", self.small.type)
end
-- TODO: comment this in
-- function immovable_property_tests:test_type_charcoal_burner()
--    assert_equal("productionsite", self.medium.type)
--    assert_equal("burners_house", self.medium.name)
-- end
function immovable_property_tests:test_type_stone()
   assert_equal("immovable", self.big.type)
end
--
-- -- ================
-- -- PlayerImmovables
-- -- ================
-- TODO: all of them are nonworking atm.
-- plr_immovables_test = lunit.TestCase("Player Immovables")
-- function plr_immovables_test:setup()
--    self.p = wl.game.Player(1)
--    self.f = self.p:place_flag(wl.map.Field(13,10), 1)
-- end
-- function plr_immovables_test:teardown()
--    pcall(self.f.remove, self.f)
-- end
--
-- function plr_immovables_test:test_owner()
--    assert_equal(self.p.number, self.f.player.number)
--    assert_equal(self.p, self.f.player)
-- end
--