# MorphRL

*MorphRL: Delving the Mutagenic Dungeon* is a traditionally-styled roguelike game. It's designed to have shorter runs and fairly horizontal character creation. The selection of equipment you can find in the dungeon is limited, but during your journey you'll obtain mutations that may grant you new abilities or improve those you already have. Or they might make things harder. Mostly, however, they just make things weirder.

While I'd been considering creating the game for some time, what prompted it's actual creation was the [Transformation Game Jam 2023](https://itch.io/jam/tf23). Due to the strict time limit on development, many aspects were specifically designed to be simpler and less ambitious than they might otherwise have been (this also aided in producing an actual, playable game).


## Building

MorphRL is built using C++ and expects the following libraries to be available:
  * BearLibTerminal    http://foo.wyrd.name/en:bearlibterminal
  * PhysicsFS          https://www.icculus.org/physfs/
  * libfov             https://github.com/google-code-export/libfov
  * stb                https://github.com/nothings/stb

Building the game is done using the included makefile; this will likely need to be edited to accommodate the install location of the aforementioned libraries.

Also included are two of the [DejaVu Fonts](https://dejavu-fonts.github.io/) (specifically the regular and oblique variants) which are used for displaying the game content.


## Contributing

The reporting of issues and feature requests is always welcome through GitHub's issue system.

Otherwise, as MorphRL is a personal, hobby project I am not looking for code contributions, though improvements to the game's technical documentation accepted.

The contribution of monster art under a suitable license is also welcome. The current design of the system requires such art to be 40x50 pixels.


## License

This project is made available under the GNU Public License (Version 3).