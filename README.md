# MorphRL

*MorphRL: Delving the Mutagenic Dungeon* is a traditionally-styled roguelike game. It's designed to have shorter runs and fairly horizontal character creation. The selection of equipment you can find in the dungeon is limited, but during your journey you'll obtain mutations that may grant you new abilities or improve those you already have. Or they might make things harder. Mostly, however, they just make things weirder.

While I'd been considering creating the game for some time, what prompted it's actual creation was the [Transformation Game Jam 2023](https://itch.io/jam/tf23). Due to the strict time limit on development, many aspects were specifically designed to be simpler and less ambitious than they might otherwise have been (this also aided in producing an actual, playable game).


## Building

MorphRL is built using C++ and uses the following libraries to be available:
  * BearLibTerminal    http://foo.wyrd.name/en:bearlibterminal
  * libfov             https://github.com/google-code-export/libfov
  * stb                https://github.com/nothings/stb

Building the game is done using the included makefile; this will likely need to be edited to accommodate the install location of the aforementioned libraries.

Also included are two of the [DejaVu Fonts](https://dejavu-fonts.github.io/) (specifically the regular and oblique variants) which are used for displaying the game content.


## Backstory

The Great War began nearly 300 years ago. Archmages fought each other, seeking to destroy their rivals and dominate the world. Decades later it ended not in victory, but with the Rending. The very forces of reality were torn apart; the lands shattered, rocks ran as water, the oceans hardened like stone, and the newborn Winds of Change ran rampant over the lands. The Winds twisted and changed everything they touched.

Thankfully, the Rendering lasted only a few years and its aftereffects have diminished over time. The surivors have learned to deal with what remains and have blamed the archmages, causing magery to be shunned and, eventually, forgotten.

The city of Sanctuary was established as the last bastion of hope for humanity (though over the past three centuries, many such "last bastions" have been discovered). A powerful barrier surrounds and protects the city and its surrounding lands from the ravages of the shattered world. The population within live in relative safety and freedom from random mutations.

Although established as a haven for humanity, it is not unwelcoming of mutates (though it's not particularly welcoming, either), though they often struggle in a world not built with them in mind. Most mutants are travelling merchants and the like.

Nothing lasts forever and the protective barrier always fades with time. About once a generation, it's necessary for someone to go beyond the barrier and retrieve fresh samples of the Ethereal Ore that powers the barrier. This time, you've been selected. You're not the first of your generation, but hopefully you will be the last. Given a sword, a potion of regeneration, and some specially designed leather armour that can adapt itself to mutations, you set out.

Fortunately, traces of Ethereal Ore have been found in a mine not far outside the barrier. Unfortunately, it lies beneath the remains of a wizard's tower, explaining why it has not been claimed before. You'll need to make your way through contamination until you can delve deep enough to claim it.


## License

This project is made available under the GNU Public License (Version 3) and all content other than the stb libraries and DejaVu fonts are made available under that license.
