dirname = path.dirname(__file__)

animations = {
   idle = {
      pictures = path.glob(dirname, "elk_idle_\\d+.png"),
      player_color_masks = {},
      hotspot = { 15, 27 },
      fps = 20,
   },
}
add_walking_animations(animations, dirname, "elk_walk", {21, 34}, 20)

world:new_critter_type{
   name = "elk",
   descname = _ "Elk",
   swimming = false,
   attributes = { "eatable" },
   programs = {
      remove = { "remove" },
   },
   animations = animations,
}
