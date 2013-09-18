Hidden Features
===============

There is a brief explanation of advanced features.

Military data
-------------
There are some features, most of them hidden by default, related to military
on the game.

Retreat when injured
^^^^^^^^^^^^^^^^^^^^
Retreat when injured, or simply retreat. When a soldier's hit points drops
below this percentage value, soldier then will try to go to safety of 
home.

``[allow_retreat_change]``
  If this section exists on config file of tribe, then player is allowed
  to change value of retreat in-game. This may be present on files like
  castle_village and headquarters_medium to activate this feature.
``[retreat_change]``
  This section encapsulates next two variables. This section should exist
  on main tribe conf file.
    
  ``retreat_change=value``
    Sets new default value for retreating, overriding default for tribe.
    This variable should exist on main tribes conf file.    
  ``retreat_interval=min-max``
    Sets valid interval of retreating for tribe that the player is allowed to
    set. This interval only is shown in game when player is allowed to modify
    retreat.
