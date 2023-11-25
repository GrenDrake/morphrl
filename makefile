CXXFLAGS=-g -Wall -pedantic -std=c++11
#-pg -no-pie

OBJS=src/startup.o src/ui_gameloop.o src/data.o src/coord.o src/dungeon.o src/mapgen.o src/image.o src/world.o src/utility.o src/fov.o src/ui_select_inventory.o src/player_actions.o src/ui_general.o src/doc_viewer.o src/ui_messagelog.o src/ui_showactor.o src/random.o src/actor.o src/item.o src/effects.o


all: morph

morph: CXXFLAGS += -I../BearLibTerminal_0.15.8/Include/C  -I../libfov/fov/ -I../physfs/src
morph: $(OBJS)
	$(CXX) $(OBJS) -L../libfov/fov/.libs -lfov -L../BearLibTerminal_0.15.8/Windows64 -lBearLibTerminal -L../physfs -lphysfs -o morph

clean:
	$(RM) src/*.o morph.exe morph

.PHONY: all clean
