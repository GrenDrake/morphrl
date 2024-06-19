
current
 * [feature] play now begins on a static opening level
 * [feature] mutations can now result in losing gained anatomy (e.g. mutating to have no wings)
 * [feature] adds pushback and stunned effects; the ramming horns gain both
 * [feature] adds ability to regen health outside of combat
 * [feature] adds "rest until healed" (R) command for restoring health and energy
 * [feature] adds option to sort player inventory
 * [balance] splits agility stat into evasion and accuracy
 * [improvement] display name of status effect when taking damage from it
 * [improvement] don't get duplicate mutations or lose features you don't have (this should also reduce or eliminate "nothing happens" results from mutations)
 * [improvement] multi-use consumables now have a visible number of remaining uses rather than a random chance to be used up
 * [improvement] displays warning on main game screen when over-burdened
 * [improvement] hostile actors will not longer spawn in stairway rooms (though they can still move into them)
 * [improvement] adds keybind menu and rebindable keys
 * [improvement] the interact action now automatically selects a target if exactly one valid target exists
 * [bugfix] actors that die from status effect now properly grant XP to their killer
 * [bugfix] special ability attacks now properly trigger on_hit effects
 * [bugfix] dropping items now clears the equipped item flag
 * [bugfix] closing the game window in the main game screen now quits rather than returning to the menu
 * [bugfix] window close button now quit while in document viewer (previously it did nothing)

alpha-2 (Dec 9, 2023)
 * [feature] adds game config file. Currently only the "fontSize" option is supported, with larger font sizes creating larger windows and vice versa.
 * [feature] adds (very) basic animations to abilities
 * [feature] allow viewing PC stats after death (but not advancing them)
 * [feature] doors that can be opened when bumped into and operated with the interact command
 * [feature] show current level when examining actors on the map
 * [balance] moves hobgoblins to second floor, displacing orcs to the third, and adds dire rats to the first level
 * [improvement] hitting ESCAPE no longer quits the game when at the main menu
 * [improvement] adds configuration option to show or hide combat math
 * [improvement] combat math display now includes the damage calculation
 * [improvement] adds config file that allows for specifying display settings before starting game
 * [improvement] use "choose direction" interface for cone effects rather than "choose target"
 * [bugfix] allow selecting tiles on right side of map with mouse
 * [bugfix] grant experience for kills made by special abilities

alpha-1 (Nov 29, 2023)
 * Initial release