#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

#include "morph.h"


    // MapgenArgs mapgenArgs{
        // 255,    // width
        // 255,    // height
        // 10000,  // roomAttempts
        // 25,     // trim dead end iterations
        // 10,     // roomFillChance
        // 40,     // roomSecondDoorChance
        // 30,     // deadEndFillChance
        // 40,     // deadEndDoorChance
        // 10,     // roomMaxWidth
        // 10,     // roomMaxHeight
        // 20,      // mazeDoorChance
        // 600,    // maxActorCount
        // 1600,   // maxItemCount

// for 63x63
const int ROOM_ATTEMPTS = 10000;
const int TRIM_DEADEND_ITERATIONS = 25;
const int ROOM_FILL_CHANCE = 10;
const int ROOM_SECOND_DOOR_CHANCE = 40;
const int DEADEND_FILL_CHANCE = 70;
const int DEADEND_DOOR_CHANCE = 15;
const int ROOM_MAX_WIDTH = 10;
const int ROOM_MAX_HEIGHT = 10;
const int MAZE_DOOR_CHANCE = 20;
const int ADD_EXTRA_DOOR_COUNT = 5;


bool isOdd(int number) {
    return number % 2;
}
bool isOdd(const Coord &c) {
    return (c.x % 2) != 0 && (c.y % 2) != 0;
}


void buildMaze(Dungeon &d) {
    Coord initial;
    do {
        initial = d.randomOfTile(TILE_UNASSIGNED);
    } while (!isOdd(initial));
    d.at(initial)->floor = TILE_FLOOR;

    std::vector<Coord> steps;
    steps.push_back(initial);

    while (!steps.empty()) {
        Coord here = steps.back();
        if (!isOdd(here)) {
            // maze squares must be on odd coordinates
            steps.pop_back();
            continue;
        }
        // pick a direction and try to tunnel in that direction. if we've
        // already got a passage in that direction, rotate direction and try
        // again. if no direction succeeds, remove this point from the list and
        // stop trying
        Direction dir = randomCardinalDirection();
        Direction wd = dir;
        bool success;
        do {
            success = false;
            Coord target = here.shift(wd, 2);
            MapTile *tile = d.at(target);
            if (!tile || tile->floor != TILE_UNASSIGNED) {
                // cant go this way
                wd = rotate90(wd);
                continue;
            }
            // we found a valid direction; build tunnel and add new point to list
            tile->floor = TILE_FLOOR;
            tile = d.at(here.shift(wd));
            if (tile) {
                if (globalRNG.upto(1000) < MAZE_DOOR_CHANCE) {
                    tile->floor = TILE_DOOR;
                } else {
                    tile->floor = TILE_FLOOR;
                }
            }
            steps.push_back(target);
            success = true;
            break;
        } while (wd != dir);

        if (!success) {
            // we didn't find any valid directions from this tile
            steps.pop_back();
        }
    }
    d.replaceTile(TILE_UNASSIGNED, TILE_WALL);
}


void createRoom(Dungeon &d, const Room &room) {
    d.fillRect(room.x, room.y, room.w, room.h, TILE_FLOOR);
    d.drawRect(room.x, room.y, room.w, room.h, TILE_WALL);
    d.addRoom(room);
}


void buildRooms(Dungeon &d) {
    int xRange = ROOM_MAX_WIDTH - 2;
    int yRange = ROOM_MAX_HEIGHT - 2;

    for (int i = 0; i < ROOM_ATTEMPTS; ++i) {
        int x = 1 + globalRNG.upto(d.width() - 2);
        x = x / 2 * 2;
        int y = 1 + globalRNG.upto(d.height() - 2);
        y = y / 2 * 2;
        int w = 3 + globalRNG.upto(xRange);
        w = w / 2 * 2 + 1;
        int h = 3 + globalRNG.upto(yRange);
        h = h / 2 * 2 + 1;

        if (d.areaIsTile(x, y, w, h, 0)) {
            createRoom(d, Room{RT_GENERIC, x, y, w, h, Direction::Unknown});
        }
    }
}


void addDoorToRoom(Dungeon &d, Room &room) {
    bool isValid;
    int iterations = 10;
    do {
        --iterations;
        isValid = false;
        Coord p = room.getPointWithin();
        // d.at(p)->floor = 3;
        Direction dir = randomCardinalDirection();
        if (dir == room.roomDirection) continue;
        while (d.isValidPosition(p) && d.floorAt(p) != TILE_WALL) {
            p = p.shift(dir);
        }
        int targetFloor = d.floorAt(p.shift(dir));
        if (targetFloor == TILE_FLOOR || targetFloor == TILE_GRASS) {
            d.floorAt(p, TILE_DOOR);
            isValid = true;
        }
    } while (iterations > 0 && !isValid);
    if (!isValid) {
        // we can't find a valid door placement, so fill the room instead
        d.fillRect(room.x, room.y, room.w, room.h, 1);
        room.isFilled = true;
        std::string errorMessage = "(failed to place door for room at ";
        errorMessage += std::to_string(room.x) + "," + std::to_string(room.y) + ")";
        logMessage(LOG_WARN, errorMessage);
    }
}


void setupRooms(Dungeon &d) {
    for (int i = 0; i < d.roomCount(); ++i) {
        Room &room = d.getRoom(i);
        if (room.w < 1) continue; // invalid room
        if (room.type == RT_GENERIC && (room.area() <= 3 || (globalRNG.upto(100)) < ROOM_FILL_CHANCE)) {
            // filled in room
            d.fillRect(room.x, room.y, room.w, room.h, 1);
            room.isFilled = true;
        } else {
            addDoorToRoom(d, room);
            if (room.type == RT_ENTRANCE || globalRNG.upto(100) < ROOM_SECOND_DOOR_CHANCE) {
                addDoorToRoom(d, room);
            }
        }

        if (room.type == RT_ENTRANCE) {
            for (int y = 0; y < room.h; ++y) {
                for (int x = 0; x < room.w; ++x) {
                    MapTile *tile = d.at(Coord(x + room.x, y + room.y));
                    if (tile) {
                        tile->temperature = 0;
                        if (tile->floor == TILE_FLOOR) tile->floor = TILE_GRASS;
                    }
                }
            }
        }
    }
}

void trimDeadEnds(Dungeon &d) {
    for (int y = 1; y < d.height(); y += 2) {
        for (int x = 1; x < d.width(); x += 2) {
            Coord here(x, y);
            if (d.floorAt(here) != TILE_FLOOR) continue;
            int wallCount = 0;
            Direction dir = Direction::North;
            Direction opening;
            do {
                Coord dest = here.shift(dir);
                if (d.floorAt(dest) == TILE_WALL) ++wallCount;
                else opening = dir;
                dir = rotate90(dir);
            } while (dir != Direction::North);
            if (wallCount == 3) {
                int roll = globalRNG.upto(100);
                if (roll < DEADEND_FILL_CHANCE) {
                    // fill in dead end
                    d.floorAt(here, TILE_WALL);
                    d.floorAt(here.shift(opening), TILE_WALL);
                } else if (roll < (DEADEND_FILL_CHANCE + DEADEND_DOOR_CHANCE)) {
                    // add door to neighboring space
                    Direction dir = randomCardinalDirection();
                    Direction workdir = dir;
                    do {
                        if (workdir != opening && d.floorAt(here.shift(workdir, 2)) == TILE_FLOOR) {
                            d.floorAt(here.shift(workdir), TILE_DOOR);
                            break;
                        }
                        workdir = rotate90(workdir);
                    } while (workdir != dir);
                }

            }
        }
    }
}

Direction getDeadEndOpening(Dungeon &d, const Coord &where) {
    int wallCount = 0;
    Direction dir = Direction::North;
    Direction opening = Direction::Unknown;
    do {
        Coord dest = where.shift(dir);
        if (d.floorAt(dest) == TILE_WALL) ++wallCount;
        else opening = dir;
        dir = rotate90(dir);
    } while (dir != Direction::North);
    if (wallCount == 3) return opening;
    return Direction::Unknown;
}

void removeDeadEnds(Dungeon &d) {
    for (int y = 1; y < d.height(); y += 2) {
        for (int x = 1; x < d.width(); x += 2) {
            Coord here(x, y);
            Direction opening;
            int iterations = 50;
            do {
                --iterations;
                opening = getDeadEndOpening(d, here);
                if (opening != Direction::Unknown) {
                    d.floorAt(here, TILE_WALL);
                    d.floorAt(here.shift(opening), TILE_WALL);
                }
                here = here.shift(opening, 2);
            } while (iterations > 0 && opening != Direction::Unknown);
        }
    }
}

void createLake(Dungeon &d) {
    int x = 1 + globalRNG.upto(d.width() - 2);
    int y = 1 + globalRNG.upto(d.height() - 2);
    Coord root(x, y);
    int r = 3 + globalRNG.upto(4);
    for (int wy = y - r; wy <= y + r; ++wy) {
        for (int wx = x - r; wx <= x + r; ++wx) {
            Coord here(wx, wy);
            double dist = root.distanceTo(here);
            if (dist > r) continue;
            d.floorAt(here, TILE_WATER);
        }
    }
};

void createTemperatureZone(Dungeon &d, int forTemp) {
    int x = 1 + globalRNG.upto(d.width() - 2);
    int y = 1 + globalRNG.upto(d.height() - 2);
    Coord root(x, y);
    int r = 3 + globalRNG.upto(10);
    for (int wy = y - r; wy <= y + r; ++wy) {
        for (int wx = x - r; wx <= x + r; ++wx) {
            Coord here(wx, wy);
            double dist = root.distanceTo(here);
            if (dist > r) continue;
            MapTile *tile = d.at(here);
            if (tile) tile->temperature = forTemp;
        }
    }
};

void addStairs(Dungeon &d) {
    bool isGood = false;

    if (d.data.hasDownStairs) {
        for (int i = 0; !isGood && i < d.roomCount(); ++i) {
            Room &room = d.getRoom(i);
            if (room.type != RT_GENERIC) continue;
            if (room.w < 4 || room.h < 4) continue;
            room.type = RT_STAIR;
            Coord stairPos(room.x + room.w / 2, room.y + room.h / 2);
            d.floorAt(stairPos, TILE_STAIR_DOWN);
            logMessage(LOG_INFO, "downstair @ " + stairPos.toString());
            isGood = true;
        }

        if (!isGood) {
            logMessage(LOG_ERROR, "Failed placing down stairs.");
        }
    }

    if (d.data.hasUpStairs) {
        isGood = false;
        for (int i = 0; !isGood && i < d.roomCount(); ++i) {
            Room &room = d.getRoom(i);
            if (room.type != RT_GENERIC) continue;
            if (room.w < 4 || room.h < 4) continue;
            room.type = RT_STAIR;
            Coord stairPos(room.x + room.w / 2, room.y + room.h / 2);
            d.floorAt(stairPos, TILE_STAIR_UP);
            logMessage(LOG_INFO, "upstair @ " + stairPos.toString());
            isGood = true;
        }

        if (!isGood) {
            logMessage(LOG_ERROR, "Failed placing up stairs.");
        }
    }
}

void addEntranceHall(Dungeon &d) {
    Room room{RT_ENTRANCE};
    Coord pos(MAP_WIDTH / 2, MAP_HEIGHT / 2);
    if (!isOdd(pos.x)) --pos.x;
    if (!isOdd(pos.y)) --pos.y;
    room.roomDirection = randomCardinalDirection();
    while (d.isValidPosition(pos.shift(room.roomDirection))) {
        pos = pos.shift(room.roomDirection);
    }

    room.x = pos.x - 5;
    room.y = pos.y - 5;
    if (isOdd(room.x)) --room.x;
    if (isOdd(room.y)) --room.y;
    room.w = room.h = 11;
    createRoom(d, room);
}

void addExtraDoors(Dungeon &d) {
    Coord where;
    bool isGood = false;
    int iterations = 10;
    do {
        --iterations;
        where.x = 2 + globalRNG.upto(d.width() - 2);
        where.y = 2 + globalRNG.upto(d.height() - 2);
        isGood = d.floorAt(where) == TILE_WALL;
        if (!isGood) continue;
        if (d.floorAt(where.shift(Direction::North)) == TILE_WALL) {
            if (d.floorAt(where.shift(Direction::South)) != TILE_WALL ||
                d.floorAt(where.shift(Direction::East))  != TILE_FLOOR ||
                d.floorAt(where.shift(Direction::West))  != TILE_FLOOR)
                isGood = false;
        } else if (d.floorAt(where.shift(Direction::East)) == TILE_WALL) {
            if (d.floorAt(where.shift(Direction::West))  != TILE_WALL ||
                d.floorAt(where.shift(Direction::North)) == TILE_FLOOR ||
                d.floorAt(where.shift(Direction::South)) == TILE_FLOOR)
                isGood = false;
        } else isGood = false;
    } while (!isGood && iterations > 0);
    if (!isGood) return;
    d.floorAt(where, TILE_DOOR);
}

void spawnActors(Dungeon &d, bool forRefresh) {
    unsigned targetCount = d.data.actorCount;
    if (forRefresh) targetCount = targetCount / 4;
    unsigned initialSpeedCounter = d.getHighestSpeedCounter();

    for (unsigned i = 0; i < targetCount; ++i) {
        bool isGood = false;
        int iterations = 20;
        Coord c;
        do {
            --iterations;
            isGood = false;
            if (iterations <= 0) break;
            c = d.randomOfTile(TILE_FLOOR);
            if (d.actorAt(c) != nullptr) continue;
            if (forRefresh && d.isSeen(c)) continue;
            isGood = true;
        } while (!isGood && iterations > 0);
        if (!isGood) continue;

        int roll = globalRNG.upto(100);
        for (const SpawnLine &line : d.data.actorSpawns) {
            if (roll < line.spawnChance) {
                Actor *actor = Actor::create(getActorData(line.ident));
                if (actor) {
                    actor->reset();
                    actor->speedCounter = initialSpeedCounter + 1;
                    d.addActor(actor, c);
                }
                break;
            }
            roll -= line.spawnChance;
        }
    }
}

void spawnItems(Dungeon &d) {
    for (unsigned i = 0; i < d.data.itemCount; ++i) {
        bool isGood = false;
        int iterations = 20;
        Coord c;
        do {
            if (iterations <= 0) break;
            c = d.randomOfTile(TILE_FLOOR);
            isGood = d.itemAt(c) == nullptr;
            --iterations;
        } while (!isGood && iterations > 0);
        if (!isGood) continue;

        int roll = globalRNG.upto(100);
        for (const SpawnLine &line : d.data.itemSpawns) {
            if (roll < line.spawnChance) {
                Item *item = new Item(getItemData(line.ident));
                if (item) {
                    d.addItem(item, c);
                }
                break;
            }
            roll -= line.spawnChance;
        }
    }
}

void doMapgen(Dungeon &d) {
    logMessage(LOG_INFO, "MAPGEN for " + std::to_string(d.depth()));
    // if we're on the ground floor, create the entrance room
    if (d.data.hasEntrance) addEntranceHall(d);
    buildRooms(d);
    addStairs(d);
    buildMaze(d);
    // createTemperatureZone(d, -1);
    // createTemperatureZone(d, 1);
    setupRooms(d);
    removeDeadEnds(d);
    // for (int i = 0; i < TRIM_DEADEND_ITERATIONS; ++i) {
        // trimDeadEnds(d);
    // }
    for (int i = 0; i < ADD_EXTRA_DOOR_COUNT; ++i) {
        addExtraDoors(d);
    }
    // for (int i = 0; i < 50; ++i) {
        // createLake(d);
    // }
    spawnActors(d, false);
    spawnItems(d);
}
