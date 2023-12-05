#include <iostream>
#include "morph.h"

World::World()
: map(nullptr), currentTurn(0), disableFOV(false), showCombatMath(true), gameState(GameState::Normal)
{ }

World::~World() {
    if (map) delete map;
}

Dungeon* World::getDungeon(int depth) {
    for (Dungeon *d : levels) {
        if (d->depth() == depth) return d;
    }
    // otherwise existing level not found, create new
    const DungeonData &dungeonData = getDungeonData(depth);
    if (dungeonData.ident == BAD_VALUE) {
        logMessage(LOG_ERROR, "Failed to find dungeon for depth " + std::to_string(depth));
        return nullptr;
    }
    int iteration = -1;
    auto oldSeed = globalRNG.getState();
    while (1) {
        ++iteration;
        if (iteration > 50) {
            logMessage(LOG_ERROR, "Unable to generate dungeon for depth " + std::to_string(depth));
            return nullptr;
        }
        Dungeon *newMap = new Dungeon(dungeonData, MAP_WIDTH, MAP_HEIGHT);
        if (!newMap) return nullptr;
        globalRNG.seed(gameSeed + dungeonData.ident + iteration);
        doMapgen(*newMap);
        // verify map connectivity
        Coord entrance = newMap->firstOfTile(TILE_GRASS);
        Coord downStair = newMap->firstOfTile(TILE_STAIR_DOWN);
        Coord upStair = newMap->firstOfTile(TILE_STAIR_UP);
        Coord startPos = entrance;
        if (startPos.x < 0) startPos = downStair;
        if (startPos.x < 0) startPos = upStair;
        if (startPos.x < 0) {
            logMessage(LOG_ERROR, "Failed to find entrance, or up or down stair");
            continue;
        }
        newMap->calcDistances(startPos);
        // ensure all exits are accessable
        if ( (entrance.x >= 0 && newMap->distanceAt(entrance) < 0) ||
             (upStair.x >= 0 && newMap->distanceAt(upStair) < 0) ||
             (downStair.x >= 0 && newMap->distanceAt(downStair) < 0) ) {
            logMessage(LOG_ERROR, "map connectivity failed");
            continue;
        }

        globalRNG.seed(oldSeed);
        levels.push_back(newMap);
        return newMap;
    }
}

bool World::movePlayerToDepth(int newDepth, int enterFrom) {
    // don't move to current depth
    if (map && newDepth == map->depth()) return true;

    Dungeon *newMap = getDungeon(newDepth);
    if (!newMap) return false; // failed to fetch new map

    if (map) {
        map->removeActor(player);
    }
    map = newMap;

    // move player to start position
    Coord startPosition(-1, -1);
    if (enterFrom == DE_ENTRANCE) {
        const Room &startRoom = map->getRoomByType(RT_ENTRANCE);
        if (startRoom.type == RT_ENTRANCE) {
            int iterations = 1000;
            do {
                --iterations;
                startPosition = startRoom.getPointWithin();
                if (!map->isValidPosition(startPosition)) {
                    startPosition.x = -1;
                    continue;
                }
                const MapTile *tile = map->at(startPosition);
                const TileData &td = getTileData(tile->floor);
                if (!td.isPassable || tile->actor) startPosition.x = -1;
            } while (iterations > 0 && startPosition.x < 0);
        } else logMessage(LOG_ERROR, "Failed to find entrance room");
    } else if (enterFrom == DE_UPSTAIRS) {
        startPosition = map->firstOfTile(TILE_STAIR_DOWN);
    } else if (enterFrom == DE_DOWNSTAIRS) {
        startPosition = map->firstOfTile(TILE_STAIR_UP);
    } else {
        logMessage(LOG_ERROR, "unknown dungeon enterFrom value " + std::to_string(enterFrom));
        return false;
    }

    if (startPosition.x < 0) {
        logMessage(LOG_ERROR, "failed to find valid start position.");
        startPosition.x = MAP_WIDTH / 2;
        startPosition.y = MAP_HEIGHT / 2;
        return false;
    }
    logMessage(LOG_INFO, "initial position @ " + startPosition.toString());
    map->addActor(player, startPosition);
    map->resetSpeedCounter();
    map->doActorFOV(player);
    return true;
}

void World::addMessage(const std::string &text) {
    messages.push_back(LogMessage{text});
}

void World::tick() {
    if (map) {
        map->tick(*this);
        ++currentTurn;
        map->doActorFOV(player);
    }
}
