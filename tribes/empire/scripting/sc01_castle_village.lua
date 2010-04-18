-- =======================================================================
--                 Empire Castle Village Starting Conditions                
-- =======================================================================

use("aux", "infrastructure")

set_textdomain("tribe_empire")

return {
   name = _ "Castle village",
   func = function(plr)
   plr:allow_workers("all")

   local sf = plr.starting_field
   local h = plr:place_building("castle", sf)
   h:set_soldiers{[{0,0,0,0}] = 12}

   place_building_in_region(plr, "warehouse", sf:region(7), { 
      wares = {
         helm = 2,
         wood_lance = 5,
         axe = 6,
         bakingtray = 2,
         basket = 1,
         fire_tongs = 2,
         fishing_rod = 2,
         flour = 4,
         grape = 4,
         hammer = 12,
         hunting_spear = 2,
         iron = 4,
         ironore = 5,
         kitchen_tools = 4,
         marble = 25,
         marblecolumn = 6,
         meal = 4,
         pick = 14,
         ration = 12,
         saw = 3,
         scythe = 5,
         shovel = 6,
         stone = 40,
         trunk = 29,
         water = 12,
         wheat = 4,
         wine = 8,
         wood = 37,
         wool = 2,
      },
      workers = {
         brewer = 1,
         builder = 10,
         burner = 1,
         carrier = 39,
         geologist = 4,
         lumberjack = 3,
         miner = 4,
         stonemason = 1,
         toolsmith = 1,
         donkey = 5,
      },
      soldiers = {
         [{0,0,0,0}] = 33, 
      }
   })
   
   place_building_in_region(plr, "colosseum", sf:region(11), {
      wares = {
         bread = 8,
         fish = 4,
         meat = 4,
      }, 
   })

   place_building_in_region(plr, "trainingscamp", sf:region(11), {
      wares = {
         fish = 2,
         meat = 2,
         helm = 2,
      },
   })

   place_building_in_region(plr, "armoursmithy", sf:region(11), {
      wares = {
            gold = 4,
            coal = 8,
            cloth = 5,
      }
   })
         
   place_building_in_region(plr, "toolsmithy", sf:region(11), {
      wares = {
         iron = 8,
      }
   })
      
   place_building_in_region(plr, "weaponsmithy", sf:region(11), {
      wares = {
         coal = 4,
         wood = 8,
      }
   })
   
   place_building_in_region(plr, "sawmill", sf:region(11), {
      wares = {
         trunk = 1,
      }
   })

   place_building_in_region(plr, "stonemasons_house", sf:region(11))
end
}