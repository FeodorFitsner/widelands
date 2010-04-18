-- ===================
-- Testing objectives 
-- ===================
objective_tests = lunit.TestCase("Objectives")
function objective_tests:setup()
   self.p = wl.game.Player(1)
   self.o1 = self.p:add_objective(
      "test1", "Cool Objective", "which really rockz!"
   )
   self.o2 = self.p:add_objective(
      "test2", "Cooler Objective", "which really suckz!"
   )
end
function objective_tests:teardown()
  pcall(self.o1.remove, self.o1)
  pcall(self.o2.remove, self.o2)
end
function objective_tests:test_properties()
   assert_equal("test1", self.o1.name)
   assert_equal("Cool Objective", self.o1.title)
   assert_equal("which really rockz!", self.o1.body)
   assert_equal(false, self.o1.done)
   assert_equal(true, self.o1.visible)
end
function objective_tests:test_set_properties()
   self.o1.title = "Blah"
   assert_equal("Blah", self.o1.title)
   self.o1.body = "Blub"
   assert_equal("Blub", self.o1.body)
   self.o1.done = true
   assert_equal(true, self.o1.done)
   self.o1.visible = false
   assert_equal(false, self.o1.visible)
end
function objective_tests:test_comparement_of_objectives()
  assert_table(self.p.objectives)
  assert_equal(self.o1, self.p.objectives.test1)
  assert_equal(self.o2, self.p.objectives.test2)
  assert_not_equal(self.o1, self.o2)
end
function objective_tests:test_create_one_new()
   function f()
      self.p:add_objective("test1", "Already exists!", "This fails")
   end
   assert_error("Objective already exists", f)
end

function objective_tests:test_deletion()
   self.o1:remove()
   assert_equal(nil, self.p.objectives.test1)
end
function objective_tests:test_deletion_and_access()
   self.o1:remove()
   assert_error("Already removed!", function() return self.o1.name end)
end
function objective_tests:test_delete_twice()
   self.o1:remove()
   assert_error("Already removed!", function() self.o1:remove() end)
end
