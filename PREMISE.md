VERSION 2

The Great War began nearly 300 years ago. Archmages fought each other, seeking to destroy their rivals and dominate the world. Decades later it ended not in victory, but with the Rending. The very forces of reality were torn apart; the lands shattered, rocks ran as water, the oceans hardened like stone, and the newborn Winds of Change ran rampant over the lands. The Winds twisted and changed everything they touched.

Thankfully, the Rendering lasted only a few years and its aftereffects have diminshed over time. The surivors have learned to deal with what remains and have blamed the archmages, causing magery to be shunned and, eventually, forgotten.

The city of Sanctuary was established as the last bastion of hope for humanity (though over the past three centuries, many such "last bastions" have been discovered). A powerful barrier surronds and protects the city and its surronding lands from the ravages of the shattered world. The population within live in relative safety and freedom from random mutations.

Although established as a haven for humanity, it is not unwelcoming of mutates (though it's not particularly welcoming, either), though they often struggle in a world not built with them in mind. Most mutants are passer-bys, travelling merchants and the like.

Nothing lasts forever and the protective barrier always fades with time. About once a generation, it's neccesary for someone to go beyond the barrier and retrieve fresh samples of the Ethereal Ore that powers the barrier. This time, you've been selected. You're not the first of your generation, but hopefully you will be the last. Given a sword, a potion of regeneration, and some specially designed leather armour that can adapt itself to mutations, you set out.

Fortunately, traces of Ethereal Ore have been found in a mine not far outside the barrier. Unfortunately, it lies beneath the remains of a wizard's tower, explaining why it has not been claimed before. You'll need to make your way through contamination until you can delve deep enough to claim it.

--=--  --=--  --=--  --=--  --=--
VERSION 1

The Great War happened nearly 300 years ago, as the greatest of the wizards fought each other, each seeking to dominate the world and to destroy their rivals. It lasted decades, ending not in victory, but with the World Rending that tore apart the forces of reality. The lands cracked, shattering beneath people's feet, rocks ran as water while oceans hardened like stone, and the winds of change ran rampant over the lands, twisting and changing everything they touched. Thankfully, the World Rending itself lasted only a few years, and even its aftereffects have greatly diminished over time. The survivors have learned to deal with those that remain, such as the Winds of Change, but blame the now long-dead wizards for the disaster, leading to the shunning of wizardry and, eventually, to its ways being forgotten.

You come from Sanctuary, established as the last bastion of hope for humanity (though over the past two-and-a-half centuries, many such "last bastions" exist). A powerful barrier protects the city and surrounding lands from the ravages of the twisted world, allowing the population within to live in relative safety and unmutated. And though it was established as a sanctuary for humanity, mutants are not unwelcome (though also not encouraged to immigrate: many of the mutants seen are travelling merchants and the like), though they often struggle in a world not built with them in mind.

But nothing lasts forever, and even the protective barriers fade with time. Once a generation or so, it becomes necessary for some to go beyond the barrier and seek out fresh samples of the Ethereal Ore that powers it. Fortunately, a mine exists not far outside the barrier. Unfortunately, it's beneath the remains of a wizard's tower and thus especially contaminated and filled with monsters. That contamination is what has allowed the Etheric Ore located there to maintain its potency.

And now it's your turn to make the plunge. You aren't the first this generation to do so, but hopefully you will be the last. Given a sword, a lesser potion of regeneration, and specially designed leather armour that can adapt to most mutations, you set out.


--=--  --=--  --=--  --=--  --=--

Dungeon
    ident / depth
    actor_spawntable_line
    item_spawntable_line




Weapons
- multiple types exist (swords, daggers, polearms, etc.)
- different types have different stats, but are mostly similar
- chance for weapons to have 1+ random enchantments?

Talismans
- can be equipped to have effect applied; do nothing when not equipped
- can have multiple effects attached

Consumables
- cannot be equipped, only used directly in inventory
- cause one or more effects when consumed

Mutations
- cannot be equipped or unequipped (but count as "equipped" when present)
- gained and removed through mutation system

Status Effects
- typically applied as consequence of other abilities
- typically use the ON_TICK effect type


EffectType
    int trigger;        // BOOST, GIVE_ABILITY, ON_HIT, ON_USE, ON_TICK
    int effectChance;
    int effectId;       // internal ID mapped to exact effect
    int effectStrength;

const int ET_BOOST = 0;
// provides a constant, static boost to an actor's stats
// effectId = statNumber, effectStrength = amount of boost
const int ET_GIVE_ABILITY
// grants a specific ability as long as the source is equipped this ability may
// be triggered by the actor as their action for the turn
// effectId = ability to give
const int ET_ON_HIT
// triggers a special effect every time the actor makes a successful attack
const int ET_ON_USE          
// triggers a special effect when the item is used from the inventory (item may be destroyed)
const int ET_ON_TICK
// triggers a special effect at the end of the actor's turn, every turn



MutationData
    unsigned ident;
    std::string name;
    std::string desc;
    slot // what part of the body is mutated? (arms, tail, etc.) or 0 for "minor" muations that do not require a slot
    bonuses...
// mutations are considered "equipped" when gained and "unequipped" when lost

StatusData
    unsigned ident;
    std::string name;
    std::string desc;
    duration min, max // can never have effect for longer than this; set very high (4294967295) to allow "permenant" effects
    resistDC // the DC required to resist the effect
    resistEveryTurn // should the actor retry the resistance until cured?
    bonuses...



ItemData/MutationData/StatusData
    bonuses
        type
            STATIC_BONUS,toWhat,amount 
            - provides bonus to stat when item is equipped
            - no effect on non-equipables
            GIVE_ABILITY,abilityId
            - when equipped, allows the use of a specific special ability
            ON_EQUIP,effectId,effectStrength
            - applies status effect when item is worn
            - effect is removed when item is removed
            ON_HIT,who,effectId,effectChance,effectStrength
            - chance to apply status effect on successful hit (odds set by `effectChance`)
            - can apply to target or user(determined by `who` variable)
            ON_USE,oneTime,effectId,effectStrength
            - triggers effect when item is activated
            - if `oneTime` is true, destroy item when used
            ON_TICK,...
            - triggers once every turn, at the end of the turn

mutations, status effects, and equipment use the same bonus data
- count mutations as "equipped" when gained
- mutations cannot be "used" - have mutations give specific abilities to simulate





--=--  --=--  --=--  --=--  --=--



The Morph Games are played in a large, maze-like dungeon/arena. Scattered throughout are numerous chests, traps, monsters, and even the occasional friendly being. The goal is to collect the magic rings; once all the rings are collected, the game ends.

The map consists of a maze-like area. Scattered throughout are numerous "rooms" - large open areas, some of which might have a very different asthethic to the dungeon itself (such as a small forest chamber). Many dungeons have a river or lake running through them; sufficently aquatic sorts can traverse these as shortcuts, but they often have their own dangers. In a few cases, dungeons have have streams or lakes of lava either instead, or in addition to, the more traditional water.

characters have a few basic stats:
* determines physical damage, carrying capacity
* agility, determines evasion and to-hit chances
* dexterity determines ability of perform fine manipulation of objects
* toughness, determines health, recovery, and resistance to poisons

Every chest contains exactly one thing. This thing can be:
* magic talismans
* mutations
* equippables (may have minimum dexterity)
* potions/consumables (ie. healing, etc.)
* worthless junk

monsters are scattered about the dungeon. There is no XP reward for combat, though they may sometimes drop non-ring chest loot. Avoiding monsters is best, but not always an option.

traps are also scattered about. they are simple affairs, and may damage the triggerer, or apply the effect of a mutation, or potion, or poison.

poison causes persistant damage, until it wears off with a successful toughness check; also initial check on application

curses are negative status effects that cause negative effects for characters, or even effect what actions they can take. curses are resisted on application with willpower, and when attemptign a prohibited action; lasts until successful check.

rooms typically contain multiple traps and monsters, but also multiple chests making them high risk, high reward

