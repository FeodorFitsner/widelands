dirname = path.dirname(__file__)

animations = {
   idle = {
      pictures = { dirname .. "idle.png" },
      hotspot = { 36, 86 }
   },
}

world:new_immovable_type{
   name = "winterland_stones2",
   descname = _ "Stones 2",
   editor_category = "stones",
   size = "big",
   attributes = { "stone" },
   programs = {
      shrink = {
         "transform=winterland_stones1"
      }
   },
   animations = animations
}
