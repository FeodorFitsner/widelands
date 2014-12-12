dirname = path.dirname(__file__)

tribes:new_immovable_type {
   name = "field_tiny",
   -- TRANSLATORS: This is an immovable name used in lists of immovables
   descname = _"Field (tiny)",
   size = "small",
   programs = {
		program = {
			"animate=idle 30000",
			"transform=field_small",
      }
   },
   helptext = {
		default = ""
   },
   animations = {
      idle = {
         pictures = path.list_directory(dirname, "idle_\\d+.png"),
         hotspot = { 11, 5 },
         fps = 5,
      },
   }
}
