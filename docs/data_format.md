# Data Files

Data files for game content are expected to be located in the `resources` subdirectory. On startup, the game will read the `game.dat` file, which may include other files to be processed as well. The format of the file is a series of directives.

Most directives can be followed by multiple properties. Many properties have default values if they aren't specified; those that lack one will show an &mdash; instead. Some properties (such as `name` or `description`) expect strings. Rather than being enclosed in quotes, these should have any whitespace replaced with underscores (`_`). For example:
```
description This_is_the_description_of_a_thing.
```


## @include [filepath]

Includes the content of another file. This temporarily suspends processing of the current file and immediately processes the named file instead.


## @define [symbol] [value]

Creates a symbol with a simple integer value. This symbol may be used anywhere that an integer is expected. Symbols must be unique across the file.


## @actor [symbol] [ident]

Creates an actor definition with the specified ident value. This ident must be unique across all actor definitions. A symbol will automatically be created with the ident value as well; if this is not wanted, a single hyphen (`-`) may be given as a name.

| Property      | Default     | Description |
| ------------- | ----------- | ----------- |
| glyph         | ?           | The character used to repersent this actor on the map. Currently, only single byte characters are permitted. |
| name          | unknown     | The name of this type of actor. |
| description   | &mdash;     | The description shown if the player examines an actor of this type. |
| colour        | 255 255 255 | The RGB components of the colour used to display actors of this kind on the map. |
| base_stats    | 0 0 0 0     | The base stats for creatures of this type, in order of strength, agility, speed, and toughness. |
| item          | &mdash;     | One or more spawnlines for items that actors of this type may be created holding. Equippable items will be automatically equipped, though if conflicting items are generated only the first will be equipped. See [spawn lines](#spawn-lines) for more information on this property's arguments. |
| mutation      | &mdash;     | One or more spawnlines for mutations this actor may possess when created. If conflicting mutations are added, later mutations will overwrite earlier ones. See [spawn lines](#spawn-lines) for more information on this property's arguments. |
| ai_mode       | AI_HOSTILE  | The AI code used for this actor, such as AI_HOSTILE or AI_PASSIVE. |
| fragile       | &mdash;     | A "fragile" actor is automatically hit and killed by any attack made on it. |
| no_refresh    | &mdash;     | Prevents this actor from being spawn while "refreshing" the population of a dungeon level. |


## @dungeon [symbol] [ident]

Creates an dungeon definition with the specified ident value. This ident must be unique across all dungeon definitions. A symbol will automatically be created with the ident value as well; if this is not wanted, a single hyphen (`-`) may be given as a name.

Actors and items being spawned are generated independantly of each other and thus spawn groups are currently not supported. The spawn group argument of `actor` and `item` lines should be set to 0.

| Property      | Default     | Description |
| ------------- | ----------- | ----------- |
| name          | unknown     | The name of this dungeon level. |
| actorCount    | 0           | The number of actors to generate on this level when created. |
| itemCount     | 0           | The number of items to generate on this level when created. |
| hasEntrance   | false       | This dungeon level contains the dungeon entrance. Only one dungeon level should have this flag. |
| noUpStairs    | false       | Do not generate stairs up on this level. |
| noDownStairs  | false       | Do not generate stairs down on this level. |
| actor         | &mdash;     | One or more spawnlines for actors to generate on this level. See [spawn lines](#spawn-lines) for more information on this property's arguments. |
| item         | &mdash;     | One or more spawnlines for items to generate on this level. See [spawn lines](#spawn-lines) for more information on this property's arguments. |


## @item [symbol] [ident]

Creates an item definition with the specified ident value. This ident must be unique across all item definitions. A symbol will automatically be created with the ident value as well; if this is not wanted, a single hyphen (`-`) may be given as a name.

| Property      | Default     | Description |
| ------------- | ----------- | ----------- |
| glyph         | ?           | The character used to repersent this item on the map. Currently, only single byte characters are permitted. |
| name          | unknown     | The name of this type of item. |
| description   | &mdash;     | The description shown if the player examines an item of this type. |
| colour        | 255 255 255 | The RGB components of the colour used to display items of this kind on the map. |
| consumeChance | 0           | The percent chance this item will be used up when used. Only applies to items of the consumable type. |
| type          | junk        | What kind of item this is (weapon, consumable, etc.). This impacts how the item can be used by actors. |
| bulk          | 1           | How much space this item takes up in an actor's inventory. |
| damage        | 0 0         | The damage range used by a weapon, in the form of minimum damage and maximum damage. |
| effect        | &mdash;     | One or more special effects possessed by this item. See the [effects section](#effects) for more detail on this property. |


## @mutation [symbol] [ident]

Creates an mutation definition with the specified ident value. This ident must be unique across all mutation definitions. A symbol will automatically be created with the ident value as well; if this is not wanted, a single hyphen (`-`) may be given as a name.

| Property      | Default     | Description |
| ------------- | ----------- | ----------- |
| gainVerb      | gaining     | The verb used when the player gains this mutation. (You mutate, *growing* feathers.) |
| name          | unknown     | The name of this mutation. |
| description   | &mdash;     | The description shown if the player examines a mutation of this type. |
| slot          | 0           | The 'slot' this mutation occupies; an actor cannot have multiple mutations that fill the same slot, other than 0. |
| effect        | &mdash;     | One or more special effects possessed by this item. See the [effects section](#effects) for more detail on this property. |


## @status [symbol] [ident]

Creates a status condition with the specified ident value. This ident must be unique across all status condition definitions. A symbol will automatically be created with the ident value as well; if this is not wanted, a single hyphen (`-`) may be given as a name.

| Property      | Default     | Description |
| ------------- | ----------- | ----------- |
| name          | unknown     | The name of this status condition. |
| description   | &mdash;     | The description shown if the player examines an status condition of this type. Currently unused. |
| minDuration   | 0           | The minimum number of rounds this condition will last before fading. |
| maxDuration   | 4294967295  | The maximum number of rounds this condition will last before fading. |
| resistDC      | 9999        | The DC of checks to resist receiving this status condition. This is checked against toughness for all statuses, including positive ones, and only applied if the check fails (very high values will make succeeding the resistance check impossible).
| effect        | &mdash;     | One or more special effects possessed by this status condition. See the [effects section](#effects) for more detail on this property. |


## @tile [symbol] [ident]

Creates an tile definition with the specified ident value. This ident must be unique across all tile definitions. A symbol will automatically be created with the ident value as well; if this is not wanted, a single hyphen (`-`) may be given as a name.

| Property      | Default     | Description |
| ------------- | ----------- | ----------- |
| glyph         | ?           | The character used to repersent this tile on the map. Currently, only single byte characters are permitted. |
| name          | unknown     | The name of this tile. |
| description   | &mdash;     | The description shown if the player examines an tile of this type. Currently unused. |
| colour        | 255 255 255 | The RGB components of the colour used to display tiles of this kind on the map. |
| isOpaque      | false       | Indicates that the tile cannot be seen through. |
| isPassable    | false       | Indicates that characters can move through this tile. |
| isUpStair     | false       | Indicates this tile is an stairway the player can use to ascend in the dungeon. |
| isDownStair   | false       | Indicates this tile is an stairway the player can use to descend in the dungeon. |


## Spawn Lines

Regardless of what kind of definition they're attached to, spawn lines always have the same basic form:
```
item [group] [chance] [ident]
```

| Argument | Description |
| -------- | ----------- |
| `chance` | The percent chance of this result being generated. |
| `group`  | The spawn group this line belongs to; only one result will be generated from spawn groups other than zero. |
| `ident`  | The ident value of the thing to be spawned. |

When generating results for a non-zero spawn group, the percent chance is only evaluated once. That is, for the following spawn lines
```
item 2 33 ITM_TALIS_EVASION
item 2 33 ITM_TALIS_HEALTH
```
there is a 33% chance of getting the first item, a 33% chance of the second item, and a 33% chance of generating nothing at all.

## Effects

An effect is a special effect caused by an item, status condition, or mutation. Effects always have the same basic form:
```
effect [trigger] [chance] [effect-id] [strength] [param]
```
effect          ET_BOOST 100 STAT_TO_HIT -1 0

| Argument    | Description |
| ----------- | ----------- |
| `trigger`   | How this effect is triggered (on-use, on-hit, per-tick, etc.) |
| `chance`    | The percent chance of this effect being triggered. |
| `effect-id` | The internal id for the effect to trigger, or the stat of boost for the boost trigger. |
| `strength`  | The strength of the effect. Exact meaning varies depending on the type of effect. |
| `param`     | Currently unused and should be set to zero. |

