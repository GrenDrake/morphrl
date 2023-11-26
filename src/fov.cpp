#include <iostream>
#include "fov.h"
#include "morph.h"


static bool testTileOpaque(void *map, int x, int y) {
    Dungeon *dungeon = static_cast<Dungeon*>(map);
    int tile = dungeon->floorAt(Coord(x, y));
    const TileData &td = getTileData(tile);
    return td.isOpaque;
}
static void setTileSeen(void *map, int x, int y, int dx, int dy, void *src) {
    Dungeon *dungeon = static_cast<Dungeon*>(map);
    dungeon->setSeen(Coord(x, y));
}


static void setTileSeenCalc(void *map, int x, int y, int dx, int dy, void *src) {
    Dungeon *dungeon = static_cast<Dungeon*>(map);
    MapTile *tile = dungeon->at(Coord(x, y));
    if (tile) tile->inFovCalc = true;
}

void handlePlayerFOV(Dungeon *dungeon, Actor *player) {
    fov_settings_type fovSettings;
    fov_settings_init(&fovSettings);
    fov_settings_set_opacity_test_function(&fovSettings, testTileOpaque);
    fov_settings_set_apply_lighting_function(&fovSettings, setTileSeen);

    dungeon->setSeen(player->position);
    void *theMap = static_cast<void*>(dungeon);
    const int playerX = player->position.x;
    const int playerY = player->position.y;
    fov_circle(&fovSettings, theMap, nullptr, playerX, playerY, 999);

    fov_settings_free(&fovSettings);
}


fov_direction_type directionToFovDir(Direction dir) {
    switch(dir) {
        case Direction::North: return FOV_NORTH;
        case Direction::Northeast: return FOV_NORTHEAST;
        case Direction::East: return FOV_EAST;
        case Direction::Southeast: return FOV_SOUTHEAST ;
        case Direction::South: return FOV_SOUTH;
        case Direction::Southwest: return FOV_SOUTHWEST ;
        case Direction::West: return FOV_WEST;
        case Direction::Northwest: return FOV_NORTHWEST;
        default: return FOV_NORTH;
    }
}

void fovCalcBeam(Dungeon *dungeon, const Coord &origin, Direction dir, int maxRange) {
    fov_settings_type fovSettings;
    fov_settings_init(&fovSettings);
    fov_settings_set_opacity_test_function(&fovSettings, testTileOpaque);
    fov_settings_set_apply_lighting_function(&fovSettings, setTileSeenCalc);

    setTileSeenCalc(static_cast<void*>(dungeon), origin.x, origin.y, 0, 0, nullptr);
    void *theMap = static_cast<void*>(dungeon);
    fov_beam(&fovSettings, theMap, nullptr, origin.x, origin.y, maxRange, directionToFovDir(dir), 90);

    fov_settings_free(&fovSettings);
}

void fovCalcBurst(Dungeon *dungeon, const Coord &origin, int maxRange) {
    fov_settings_type fovSettings;
    fov_settings_init(&fovSettings);
    fov_settings_set_opacity_test_function(&fovSettings, testTileOpaque);
    fov_settings_set_apply_lighting_function(&fovSettings, setTileSeenCalc);

    setTileSeenCalc(static_cast<void*>(dungeon), origin.x, origin.y, 0, 0, nullptr);
    void *theMap = static_cast<void*>(dungeon);
    fov_circle(&fovSettings, theMap, nullptr, origin.x, origin.y, maxRange);

    fov_settings_free(&fovSettings);
}



static bool lineShouldStop(const Dungeon &map, const Coord &where, bool stopOpaque, bool stopSolid) {
    if (!map.isValidPosition(where)) return true;
    int tile = map.floorAt(where);
    const TileData &td = getTileData(tile);
    if (stopOpaque && td.isOpaque) return true;
    if (stopSolid && !td.isPassable) return true;
    return false;

}
std::vector<Coord> calcLine(const Dungeon &map, const Coord &start, const Coord &end, bool stopOpaque, bool stopSolid) {
    std::vector<Coord> results;

    int dx = end.x - start.x;
    signed char const ix = (dx > 0) - (dx < 0);
    dx = abs(dx) << 1;

    int dy = end.y - start.y;
    signed char iy = (dy > 0) - (dy < 0);
    dy = abs(dy) << 1;

    results.push_back(start);

    Coord pos = start;
    if (dx > dy) {
        int error = dy - (dx >> 1);
        if (pos.x != end.x) while (1) {
            if ((error > 0) || (!error && (ix > 0))) {
                error -= dx;
                pos.y += iy;
            }
            error += dy;
            pos.x += ix;
            results.push_back(pos);
            if (lineShouldStop(map, pos, stopOpaque, stopSolid)) break;
        }
    } else {
        int error(dx - (dy >> 1));
        if (pos.y != end.y) while (1) {
            if ((error > 0) || (!error && (iy > 0))) {
                error -= dy;
                pos.x += ix;
            }
            error += dx;
            pos.y += iy;
            results.push_back(pos);
            if (lineShouldStop(map, pos, stopOpaque, stopSolid)) break;
        }
    }

    return results;
}


