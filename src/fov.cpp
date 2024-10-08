#include <cmath>
#include <vector>
#include "morph.h"


/* ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** *****
 * GENERAL FOV FUNCTIONS
 * ADAPTED FROM:
 * http://www.adammil.net/blog/v125_Roguelike_Vision_Algorithms.html
 * ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** *****/

struct LevelPoint {
    int X, Y;
};

// represents the slope Y/X as a rational number
struct Slope  {
    Slope(int y, int x) { Y=y; X=x; }
    bool Greater(int y, int x) { return Y*x > X*y; } // this > y/x
    bool GreaterOrEqual(int y, int x) { return Y*x >= X*y; } // this >= y/x
    bool LessOrEqual(int y, int x) { return Y*x <= X*y; } // this <= y/x
    int Y, X;
};

class FovCalc {
public:
    Dungeon *map;
    bool forCalc;

    FovCalc(Dungeon *map, bool forCalc) : map(map), forCalc(forCalc) { }
    virtual void Compute(LevelPoint origin, int rangeLimit) = 0;

    /// A function that accepts the X and Y coordinates of a tile and determines
    /// whether the given tile blocks the passage of light.
    bool BlocksLight(int x, int y);

    /// A function that takes the X and Y coordinate of a point where X >= 0,
    /// Y >= 0, and X >= Y, and returns the distance from the point to the origin (0,0).
    int GetDistance(int x, int y);

    /// A function that sets a tile to be visible, given its X and Y coordinates.
    void SetVisible(int x, int y);
};

class ShadowCastVisibility : public FovCalc{
public:
    ShadowCastVisibility(Dungeon *map, bool forCalc) : FovCalc(map, forCalc) { }
    virtual void Compute(LevelPoint origin, int rangeLimit) override;
private:
    void Compute(uint octant, LevelPoint origin, int rangeLimit, int x, Slope top, Slope bottom);
};

class DiamondWallsVisibility : public FovCalc {
public:
    Dungeon *map;
    bool forCalc;

    DiamondWallsVisibility(Dungeon *map, bool forCalc) : FovCalc(map, forCalc) { }

    void Compute(unsigned octant, LevelPoint origin, int rangeLimit, int x, Slope top, Slope bottom);
    void Compute(LevelPoint origin, int rangeLimit);
    bool BlocksLight(int x, int y, unsigned octant, LevelPoint origin);
};


bool FovCalc::BlocksLight(int x, int y) {
    int tile = map->floorAt(Coord(x, y));
    const TileData &td = getTileData(tile);
    return td.isOpaque;
}

int FovCalc::GetDistance(int x, int y) {
    double dist = std::sqrt(x*x + y*y);
    return dist;
}

void FovCalc::SetVisible(int x, int y) {
    if (forCalc) {
        MapTile *tile = map->at(Coord(x, y));
        if (tile) tile->inFovCalc = true;
    } else map->setSeen(Coord(x, y));
}


void ShadowCastVisibility::Compute(uint octant, LevelPoint origin, int rangeLimit, int x, Slope top, Slope bottom)
{
    // rangeLimit < 0 || x <= rangeLimit
    for(; (uint)x <= (uint)rangeLimit; x++) {
        // compute the Y coordinates where the top vector leaves the column (on the right) and where the bottom vector
        // enters the column (on the left). this equals (x+0.5)*top+0.5 and (x-0.5)*bottom+0.5 respectively, which can
        // be computed like (x+0.5)*top+0.5 = (2(x+0.5)*top+1)/2 = ((2x+1)*top+1)/2 to avoid floating point math
        int topY = top.X == 1 ? x : ((x*2+1) * top.Y + top.X - 1) / (top.X*2); // the rounding is a bit tricky, though
        int bottomY = bottom.Y == 0 ? 0 : ((x*2-1) * bottom.Y + bottom.X) / (bottom.X*2);

        int wasOpaque = -1; // 0:false, 1:true, -1:not applicable
        for(int y=topY; y >= bottomY; y--) {
            int tx = origin.X, ty = origin.Y;
            // translate local coordinates to map coordinates
            switch(octant) {
                case 0: tx += x; ty -= y; break;
                case 1: tx += y; ty -= x; break;
                case 2: tx -= y; ty -= x; break;
                case 3: tx -= x; ty -= y; break;
                case 4: tx -= x; ty += y; break;
                case 5: tx -= y; ty += x; break;
                case 6: tx += y; ty += x; break;
                case 7: tx += x; ty += y; break;
            }

            bool inRange = rangeLimit < 0 || GetDistance(x, y) <= rangeLimit;
            if(inRange) SetVisible(tx, ty);
            // NOTE: use the next line instead if you want the algorithm to be symmetrical
            // if(inRange && (y != topY || top.Y*x >= top.X*y) && (y != bottomY || bottom.Y*x <= bottom.X*y)) SetVisible(tx, ty);

            bool isOpaque = !inRange || BlocksLight(tx, ty);
            if(x != rangeLimit) {
                if (isOpaque) {
                    // if we found a transition from clear to opaque, this sector is done in this column, so
                    // adjust the bottom vector upwards and continue processing it in the next column.
                    if (wasOpaque == 0) {
                        // (x*2-1, y*2+1) is a vector to the top-left of the opaque tile
                        Slope newBottom = Slope(y * 2 + 1, x * 2 - 1);
                        // don't recurse unless we have to
                        if (!inRange || y == bottomY) { bottom = newBottom; break; }
                        else Compute(octant, origin, rangeLimit, x + 1, top, newBottom);
                    }
                    wasOpaque = 1;
                }
                // adjust top vector downwards and continue if we found a transition from opaque to clear
                // (x*2+1, y*2+1) is the top-right corner of the clear tile (i.e. the bottom-right of the opaque tile)
                else {
                    if (wasOpaque > 0) top = Slope(y*2+1, x*2+1);
                    wasOpaque = 0;
                }
            }
        }

        // if the column ended in a clear tile, continue processing the current sector
        if(wasOpaque != 0) break;
    }
}

void ShadowCastVisibility::Compute(LevelPoint origin, int rangeLimit) {
    SetVisible(origin.X, origin.Y);
    for(uint octant = 0; octant < 8; octant++) {
        Compute(octant, origin, rangeLimit, 1, Slope(1, 1), Slope(0, 1));
    }
}

void DiamondWallsVisibility::Compute(unsigned octant, LevelPoint origin, int rangeLimit, int x, Slope top, Slope bottom) {
    // rangeLimit < 0 || x <= rangeLimit
    for(; (unsigned)x <= (unsigned)rangeLimit; x++) {
        int topY;
        if (top.X == 1) {
            topY = x;
        } else {
            // get the tile that the top vector enters from the left
            topY = ((x * 2 - 1) * top.Y + top.X) / (top.X * 2);
            int ay = (topY * 2 + 1) * top.X;
            if (BlocksLight(x, topY, octant, origin)) { // if the top tile is a wall...
                // but the top vector misses the wall and passes into the tile above, move up
                if (top.GreaterOrEqual(ay, x*2)) topY++;
            } else { // the top tile is not a wall
                // so if the top vector passes into the tile above, move up
                if (top.Greater(ay, x*2+1)) topY++;
            }
        }

        int bottomY = bottom.Y == 0 ? 0 : ((x*2-1) * bottom.Y + bottom.X) / (bottom.X*2);
        int wasOpaque = -1; // 0:false, 1:true, -1:not applicable
        for (int y = topY; y >= bottomY; y--) {
            int tx = origin.X, ty = origin.Y;
            // translate local coordinates to map coordinates
            switch(octant) {
                case 0: tx += x; ty -= y; break;
                case 1: tx += y; ty -= x; break;
                case 2: tx -= y; ty -= x; break;
                case 3: tx -= x; ty -= y; break;
                case 4: tx -= x; ty += y; break;
                case 5: tx -= y; ty += x; break;
                case 6: tx += y; ty += x; break;
                case 7: tx += x; ty += y; break;
            }

            bool inRange = rangeLimit < 0 || GetDistance(x, y) <= rangeLimit;
            // NOTE: use the following line instead to make the algorithm symmetrical
            if (inRange && (y != topY || top.GreaterOrEqual(y, x)) && (y != bottomY || bottom.LessOrEqual(y, x))) SetVisible(tx, ty);
            // if (inRange) SetVisible(tx, ty);

            bool isOpaque = !inRange || FovCalc::BlocksLight(tx, ty);
            // if y == topY or y == bottomY, make sure the sector actually intersects the wall tile. if not, don't consider
            // it opaque to prevent the code below from moving the top vector up or the bottom vector down
            if (isOpaque &&
               ((y == topY && top.LessOrEqual(y*2-1, x*2) && !BlocksLight(x, y-1, octant, origin)) ||
               (y == bottomY && bottom.GreaterOrEqual(y*2+1, x*2) && !BlocksLight(x, y+1, octant, origin)))) {
                isOpaque = false;
            }

            if (x != rangeLimit) {
                if (isOpaque) {
                    // if we found a transition from clear to opaque, this sector is done in this column, so
                    // adjust the bottom vector upwards and continue processing it in the next column.
                    if (wasOpaque == 0) {
                        // (x*2-1, y*2+1) is a vector to the top-left corner of the opaque block
                        if (!inRange || y == bottomY) { bottom = Slope(y*2+1, x*2); break; } // don't recurse unless necessary
                        else Compute(octant, origin, rangeLimit, x+1, top, Slope(y*2+1, x*2));
                    }
                    wasOpaque = 1;
                } else {
                    // (x*2+1, y*2+1) is the top-right corner of the clear tile (i.e. the bottom-right of the opaque tile)
                    // adjust the top vector downwards and continue if we found a transition from opaque to clear
                    if (wasOpaque > 0) top = Slope(y*2+1, x*2);
                    wasOpaque = 0;
                }
            }
        }

        if (wasOpaque != 0) break; // if the column ended in a clear tile, continue processing the current sector
    }
}

void DiamondWallsVisibility::Compute(LevelPoint origin, int rangeLimit) {
    SetVisible(origin.X, origin.Y);
    for (unsigned octant = 0; octant < 8; octant++) {
        Compute(octant, origin, rangeLimit, 1, Slope(1, 1), Slope(0, 1));
    }
}

bool DiamondWallsVisibility::BlocksLight(int x, int y, unsigned octant, LevelPoint origin) {
    int nx = origin.X, ny = origin.Y;
    switch (octant) {
        case 0: nx += x; ny -= y; break;
        case 1: nx += y; ny -= x; break;
        case 2: nx -= y; ny -= x; break;
        case 3: nx -= x; ny -= y; break;
        case 4: nx -= x; ny += y; break;
        case 5: nx -= y; ny += x; break;
        case 6: nx += y; ny += x; break;
        case 7: nx += x; ny += y; break;
    }
    return FovCalc::BlocksLight(nx, ny);
}


/* ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** *****
 * GAME-SPECIFIC FOV FUNCTIONS
 * ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** *****/

void handlePlayerFOV(Dungeon *dungeon, Actor *player) {
    DiamondWallsVisibility fov(dungeon, false);
    // ShadowCastVisibility fov(dungeon, false);
    LevelPoint p;
    p.X = player->position.x;
    p.Y = player->position.y;
    fov.Compute(p, -1);
}

const double PI_VALUE = 3.14159;
static double radiansToDegrees(double rad) {
    double work = rad * (180 / PI_VALUE);
    work -= 180;
    while (work < 0) work += 360;
    while (work >= 360) work -= 260;
    return std::round(work);
}

static bool validAngleForDirection(Direction dir, double angle) {
    switch (dir) {
        case Direction::Northwest:  return angle >= 0 && angle <= 90;
        case Direction::North:      return angle >= 45  && angle <= 135;
        case Direction::Northeast:  return angle >= 90  && angle <= 180;
        case Direction::East:       return angle >= 135 && angle <= 225;
        case Direction::Southeast:  return angle >= 180 && angle <= 270;
        case Direction::South:      return angle >= 225 && angle <= 315;
        case Direction::Southwest:  return angle >= 270 && angle <= 360;
        case Direction::West:       return angle >= 315 || angle <= 45;
        default:
            return false;
    }
}

void fovCalcBeam(Dungeon *dungeon, const Coord &origin, Direction dir, int maxRange) {
    DiamondWallsVisibility fov(dungeon, true);
    LevelPoint p;
    p.X = origin.x;
    p.Y = origin.y;
    fov.Compute(p, maxRange);

    for (int y = 0; y < dungeon->height(); ++y) {
        for (int x = 0; x < dungeon->width(); ++x) {
            MapTile *mapTile = dungeon->at(Coord(x, y));
            if (!mapTile || !mapTile->inFovCalc) continue;
            double angle = std::atan2(y - origin.y, x - origin.x);
            angle = radiansToDegrees(angle);
            if (!validAngleForDirection(dir, angle)) mapTile->inFovCalc = false;
        }
    }
}

void fovCalcBurst(Dungeon *dungeon, const Coord &origin, int maxRange) {
    DiamondWallsVisibility fov(dungeon, true);
    LevelPoint p;
    p.X = origin.x;
    p.Y = origin.y;
    fov.Compute(p, maxRange);
}


/* ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** *****
 * LINE CALC FUNCTIONS
 * ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** *****/

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


