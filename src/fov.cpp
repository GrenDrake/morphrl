#include "fov.h"
#include "morph.h"


bool testTileOpaque(void *map, int x, int y) {
    Dungeon *dungeon = static_cast<Dungeon*>(map);
    int tile = dungeon->floorAt(Coord(x, y));
    const TileData &td = getTileData(tile);
    return td.isOpaque;
}
void setTileSeen(void *map, int x, int y, int dx, int dy, void *src) {
    Dungeon *dungeon = static_cast<Dungeon*>(map);
    dungeon->setSeen(Coord(x, y));
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


