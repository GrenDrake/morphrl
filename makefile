CXXFLAGS=-Wall -pedantic -std=c++11
#-pg -no-pie

OBJS=src/startup.o src/ui_gameloop.o src/data.o src/coord.o src/dungeon.o src/mapgen.o src/image.o src/world.o src/utility.o src/fov.o src/ui_select_inventory.o src/player_actions.o src/ui_general.o src/doc_viewer.o src/ui_messagelog.o src/ui_showactor.o src/random.o src/actor.o src/item.o src/effects.o src/ui_charinfo.o src/ui_debugcodex.o src/gamelog.o src/config.o


all: debug

release: clean morph package

debug: CXXFLAGS += -DDEBUG -g
debug: morph

package:
	$(RM) -r morphrl
	mkdir morphrl
	cp game.cfg DejaVuSansMono.ttf DejaVuSansMono-Oblique.ttf morphrl
	cp morph libphysfs.dll BearLibTerminal.dll libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll morphrl
	zip -j morphrl/gamedata.dat resources/*
	zip -r morphrl.zip morphrl

morph: CXXFLAGS += -I../BearLibTerminal_0.15.8/Include/C  -I../libfov/fov/ -I../physfs/src
morph: $(OBJS)
	$(CXX) $(OBJS) -mwindows -L../libfov/fov/.libs -lfov -L../BearLibTerminal_0.15.8/Windows64 -lBearLibTerminal -L../physfs -lphysfs -o morph

clean:
	$(RM) src/*.o morph.exe morph
	$(RM) -r morphrl morphrl.zip

.PHONY: all clean
