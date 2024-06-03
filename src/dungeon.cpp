#include <map>
#include <deque>
#include <iostream>
#include <sstream>
#include <vector>

#include "morph.h"


MapTile::MapTile()
: floor(0), actor(nullptr), temperature(0), isSeen(false), everSeen(false), inFovCalc(false)
{ }


int Room::area() const {
    return (w - 2) * (h - 2);
}


Coord Room::getPointWithin() const {
    int dx = globalRNG.upto(w / 2);
    int dy = globalRNG.upto(h / 2);
    dx *= 2;
    dy *= 2;
    return Coord(1 + x + dx, 1 + y + dy);
}



Dungeon::Dungeon(const DungeonData &data, int width, int height)
: data(data), mDepth(data.ident), mWidth(width), mHeight(height)
{
    mData = new MapTile[mWidth * mHeight];
}


void Dungeon::clear() {
    if (!mData) return;
    unsigned size = mWidth * mHeight;
    for (unsigned i = 0; i < size; ++i) {
        mData[i].floor = 0;
    }
}

void Dungeon::clearDistances() {
    if (!mData) return;
    unsigned size = mWidth * mHeight;
    for (unsigned i = 0; i < size; ++i) {
        mData[i].distanceValue = -1;
    }
}


bool Dungeon::areaIsTile(int x, int y, int w, int h, int theTile) const {
    for (int wy = y; wy < y + h; wy++) {
        for (int wx = x; wx < x + w; wx++) {
            const MapTile *tile = at(Coord(wx, wy));
            if (!tile || tile->floor != theTile) return false;
        }
    }
    return true;
}


void Dungeon::drawRect(int x, int y, int w, int h, int toTile) {
    int x2 = x + w - 1;
    int y2 = y + h - 1;
    for (int wx = x; wx <= x2; ++wx) {
        MapTile *tile = at(Coord(wx, y));
        if (tile) tile->floor = toTile;
        tile = at(Coord(wx, y2));
        if (tile) tile->floor = toTile;
    }
    for (int wy = y; wy <= y2; ++wy) {
        MapTile *tile = at(Coord(x, wy));
        if (tile) tile->floor = toTile;
        tile = at(Coord(x2, wy));
        if (tile) tile->floor = toTile;
    }
}


void Dungeon::fillRect(int x, int y, int w, int h, int toTile) {
    for (int wy = y; wy < y + h; wy++) {
        for (int wx = x; wx < x + w; wx++) {
            MapTile *tile = at(Coord(wx, wy));
            if (tile) tile->floor = toTile;
        }
    }
}


void Dungeon::replaceTile(int fromTile, int toTile) {
    if (!mData) return;
    unsigned size = mWidth * mHeight;
    for (unsigned i = 0; i < size; ++i) {
        if (mData[i].floor == fromTile) {
            mData[i].floor = toTile;
        }
    }
}

void Dungeon::calcDistances(const Coord &fromWhere) {
    clearDistances();
    std::deque<Coord> worklist;
    worklist.push_front(fromWhere);
    MapTile *originTile = at(fromWhere);
    if (!originTile) return;
    originTile->distanceValue = 0;

    while (!worklist.empty()) {
        const Coord work = worklist.front();
        worklist.pop_front();
        MapTile *tileA = at(work);
        if (!tileA) continue;
        Direction dir = Direction::North;
        do {
            Coord adj = work.shift(dir);
            dir = rotate45(dir);
            MapTile *tileB = at(adj);
            if (!tileB) continue;
            if (tileB->distanceValue >= 0) continue;
            const TileData &td = getTileData(tileB->floor);
            if (!td.isPassable && td.ident != TILE_CLOSED_DOOR) continue;
            tileB->distanceValue = tileA->distanceValue + 1;
            worklist.push_back(adj);
        } while (dir != Direction::North);
    }
}

Coord Dungeon::nearestOpenTile(const Coord &source, bool allowActor, bool allowItem) const {
    if (!isValidPosition(source)) return Coord(-1, -1);
    const MapTile *originTile = at(source);
    const TileData &originData = getTileData(originTile->floor);
    if ( (allowActor || !originTile->actor) &&
         (allowItem || originTile->items.empty()) &&
         (originData.isPassable)) return source;

    Direction dir = Direction::North;
    do {
        bool isGood = true;
        const MapTile *tile = at(source.shift(dir));
        if (tile) {
            const TileData &td = getTileData(tile->floor);
            if (!td.isPassable) isGood = false;
            if (!allowActor && tile->actor) isGood = false;
            if (!allowItem && !tile->items.empty()) isGood = false;
            if (isGood) return source.shift(dir);
        }
        dir = rotate45(dir);
    } while (dir != Direction::North);

    return Coord(-1, -1);
}

Coord Dungeon::randomOpenTile(bool allowActor, bool allowItem) const {
    int x, y, iterations = 10000;
    bool isValid;
    do {
        --iterations;
        isValid = false;
        x = 1 + (globalRNG.upto(mWidth - 2));
        y = 1 + (globalRNG.upto(mHeight - 2));
        const MapTile *tile = at(Coord(x, y));
        if (!tile) continue;
        if (tile->actor) continue;
        const TileData &td = getTileData(tile->floor);
        if (!td.isPassable) continue;
        isValid = true;
    } while (iterations > 0 && !isValid);
    if (isValid) return Coord(x, y);
    return Coord(-1, -1);
}

Coord Dungeon::randomOfTile(int theTile) const {
    int x, y, iterations = 10000;
    bool isValid;
    do {
        x = 1 + (globalRNG.upto(mWidth - 2));
        y = 1 + (globalRNG.upto(mHeight - 2));
        const MapTile *tile = at(Coord(x, y));
        isValid = tile && tile->floor == theTile;
    } while (iterations > 0 && !isValid);
    if (isValid) return Coord(x, y);
    return Coord(-1, -1);
}

Coord Dungeon::firstOfTile(int theTile) const {
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            Coord here(x, y);
            if (floorAt(here) == theTile) return here;
        }
    }
    return Coord(-1, -1);
}


bool Dungeon::isValidPosition(const Coord &where) const {
    if (where.x < 0 || where.y < 0) return false;
    if (where.x >= mWidth || where.y >= mHeight) return false;
    return true;
}


unsigned Dungeon::toPosition(const Coord &where) const {
    return where.x + where.y * mWidth;
}


int Dungeon::distanceAt(const Coord &where) const {
    const MapTile *tile = at(where);
    if (!tile) return -1;
    return tile->distanceValue;
}

int Dungeon::floorAt(const Coord &where) const {
    const MapTile *tile = at(where);
    if (!tile) return TILE_UNASSIGNED;
    return tile->floor;
}

void Dungeon::floorAt(const Coord &where, int toTile) {
    MapTile *tile = at(where);
    if (!tile) return;
    tile->floor = toTile;
}

const MapTile* Dungeon::at(const Coord &where) const {
    if (!mData) return nullptr;
    if (!isValidPosition(where)) return nullptr;
    unsigned pos = toPosition(where);
    if (pos >= static_cast<unsigned>(mWidth) * static_cast<unsigned>(mHeight)) return nullptr;
    return &mData[pos];
}
MapTile* Dungeon::at(const Coord &where) {
    if (!mData) return nullptr;
    if (!isValidPosition(where)) return nullptr;
    unsigned pos = toPosition(where);
    if (pos >= static_cast<unsigned>(mWidth) * static_cast<unsigned>(mHeight)) return nullptr;
    return &mData[pos];
}

bool Dungeon::isSeen(const Coord &where) const {
    const MapTile *tile = at(where);
    if (tile) return tile->isSeen;
    return false;
}

bool Dungeon::everSeen(const Coord &where) const {
    const MapTile *tile = at(where);
    if (tile) return tile->everSeen;
    return false;
}

void Dungeon::setSeen(const Coord &where) {
    MapTile *tile = at(where);
    if (tile) {
        tile->isSeen = true;
        tile->everSeen = true;
    }
}

void Dungeon::clearIsSeen() {
    unsigned count = mWidth * mHeight;
    for (unsigned i = 0; i < count; ++i) {
        mData[i].isSeen = false;
    }
}

void Dungeon::doActorFOV(Actor *actor) {
    clearIsSeen();
    handlePlayerFOV(this, actor);
}

bool Dungeon::hostileIsVisible() const {
    for (int y = 0; y < mHeight; ++y) {
        for (int x = 0; x < mWidth; ++x) {
            Coord here(x, y);
            if (!isSeen(here)) continue;
            const Actor *who = actorAt(here);
            if (!who || who->isPlayer) continue;
            return true;
        }
    }
    return false;
}

bool Dungeon::addActor(Actor *who, const Coord &where) {
    if (!who) return false;
    MapTile *tile = at(where);
    if (!tile || tile->actor) return false;
    tile->actor = who;
    who->position = where;
    who->onMap = this;
    mActors.push_back(who);
    return true;
}

bool Dungeon::moveActor(Actor *who, const Coord &where) {
    if (!who) return false;
    MapTile *oldTile = at(who->position);
    MapTile *newTile = at(where);
    if (!oldTile || !newTile || oldTile->actor != who || newTile->actor) return false;
    oldTile->actor = nullptr;
    newTile->actor = who;
    who->position = where;
    return true;
}

bool Dungeon::tryActorStep(Actor *who, Direction dir) {
    Coord dest = who->position.shift(dir);
    int newFloor = floorAt(dest);
    const TileData &td = getTileData(newFloor);
    if (!td.isPassable) return false;
    return moveActor(who, dest);
}

bool Dungeon::tryActorStepApprox(Actor *who, Direction dir) {
    bool result = tryActorStep(who, dir);
    if (result) return true;
    result = tryActorStep(who, rotate45(dir));
    if (result) return true;
    result = tryActorStep(who, unrotate45(dir));
    if (result) return true;
    return false;
}

bool Dungeon::removeActor(Actor *who) {
    if (!who) return false;
    MapTile *tile = at(who->position);
    if (!tile) return false;
    if (tile->actor == who) tile->actor = nullptr;
    auto iter = mActors.begin();
    while (iter != mActors.end()) {
        if (*iter == who) {
            mActors.erase(iter);
            break;
        }
        ++iter;
    }
    who->position = Coord(-1, -1);
    who->onMap = nullptr;
    return true;
}

const Actor* Dungeon::actorAt(const Coord &where) const {
    const MapTile *tile = at(where);
    if (!tile) return nullptr;
    return tile->actor;
}
Actor* Dungeon::actorAt(const Coord &where) {
    MapTile *tile = at(where);
    if (!tile) return nullptr;
    return tile->actor;
}

void Dungeon::resetSpeedCounter() {
    for (Actor *actor : mActors) {
        actor->speedCounter = 0;
    }
}

bool Dungeon::addItem(Item *what, const Coord &where) {
    if (!what) return false;
    MapTile *tile = at(where);
    if (!tile) return false;
    tile->items.push_back(what);
    what->position = where;
    return true;
}

bool Dungeon::removeItem(Item *what) {
    if (!what) return false;
    MapTile *tile = at(what->position);
    if (!tile) return false;
    for (auto iter = tile->items.begin(); iter != tile->items.end(); ) {
        if (*iter == what) {
            iter = tile->items.erase(iter);
        } else {
            ++iter;
        }
    }
    what->position = Coord(-1, -1);
    return true;
}

const Item* Dungeon::itemAt(const Coord &where) const {
    const MapTile *tile = at(where);
    if (!tile) return nullptr;
    if (tile->items.empty()) return nullptr;
    return tile->items.front();
}
Item* Dungeon::itemAt(const Coord &where) {
    MapTile *tile = at(where);
    if (!tile) return nullptr;
    if (tile->items.empty()) return nullptr;
    return tile->items.front();
}


Room BAD_ROOM { -1 };
void Dungeon::addRoom(const Room &room) {
    mRooms.push_back(room);
}

const Room& Dungeon::getRoomByType(int roomType) const {
    for (const Room &room : mRooms) {
        if (room.type == roomType) return room;
    }
    return BAD_ROOM;
}

Room& Dungeon::getRoom(int index) {
    if (index >= static_cast<int>(mRooms.size())) return BAD_ROOM;
    return mRooms[index];
}

void Dungeon::clearDeadActors() {
    auto iter = mActors.begin();
    while (iter != mActors.end()) {
        if (!(*iter)->isPlayer && (*iter)->isDead()) {
            Actor *corpse = *iter;
            MapTile *tile = at(corpse->position);
            // we don't use `removeActor` here because it would invalidate
            // the iterator from this loop
            if (tile->actor == corpse) tile->actor = nullptr;
            iter = mActors.erase(iter);
            delete corpse;
        } else {
            ++iter;
        }
    }
}

Actor* Dungeon::getNextActor() {
    Actor *next = nullptr;
    for (Actor *actor : mActors) {
        if (actor == nullptr) continue;
        else if (actor->health <= 0) continue; // skip dead actors
        else if (next == nullptr) next = actor;
        else {
            if (actor->speedCounter < next->speedCounter) next = actor;
        }
    }
    return next;
}

unsigned Dungeon::getHighestSpeedCounter() const {
    unsigned highestSpeedCounter = 0;
    for (const Actor *oldActor : mActors) {
        if (oldActor->speedCounter > highestSpeedCounter) {
            highestSpeedCounter = oldActor->speedCounter;
        }
    }
    return highestSpeedCounter;
}

void Dungeon::tick(World &world) {
    const unsigned refreshFrequency = 35;
    static unsigned turnCount = 0;
    if (!overlayTiles.empty()) return;

    while (1) {
        if (turnCount >= refreshFrequency) {
            turnCount = 0;
            if (mActors.size() < data.actorCount / 2) {
                spawnActors(*this, true);
            }
        }

        if (world.player->isDead()) {
            clearDeadActors();
            return;
        }
        Actor *actor = getNextActor();
        actor->verify();

        ++actor->turnsSinceCombatAction;
        if (actor->turnsSinceCombatAction > 5) actor->takeDamage(-1, actor);
        bool isStunned = false;
        std::stringstream msg;
        auto statusIter = actor->statusEffects.begin();
        while (statusIter != actor->statusEffects.end()) {
            StatusItem *status = *statusIter;
            ++status->duration;
            if (status->duration <= 1) continue; // don't apply status conditions on the first turn they're received

            for (const EffectData &effect : status->data.effects) {
                if (effect.trigger == ET_STUN) {
                    isStunned = true;
                    msg << ucFirst(actor->getName()) << " is stunned and cannot act! ";
                    continue;
                }
                if (effect.trigger != ET_ON_TICK) continue;
                std::string resultString = triggerEffect(effect, status->fromWho, actor);
                if (!resultString.empty()) {
                    msg << ucFirst(actor->getName()) << " is effected by " << status->data.name << ". ";
                    msg << resultString;
                }
            }

            // check for effect expiry
            if (status->duration >= status->data.maxDuration) {
                statusIter = actor->statusEffects.erase(statusIter);
                if (actor->isPlayer) {
                    if (actor->isPlayer) msg << "Your [color=yellow]" << status->data.name << "[/color] fades. ";
                    else msg << ucFirst(actor->getName(true)) + "'s " << status->data.name << " fades. ";
                }
            } else ++statusIter;
        }
        for (const MutationItem *mutationItem : actor->mutations) {
            for (const EffectData &effect : mutationItem->data.effects) {
                if (effect.trigger != ET_ON_TICK) continue;
                std::string resultString = triggerEffect(effect, actor, nullptr);
                if (!resultString.empty()) {
                    msg << resultString;
                }
            }
        }
        int energyToRecover = actor->getStat(STAT_ENERGY) / 20;
        if (energyToRecover < 1) energyToRecover = 1;
        actor->spendEnergy(-energyToRecover);

        const std::string &text = msg.str();
        if (!text.empty()) world.addMessage(text);

        actor->verify();
        if (isStunned) {
            actor->advanceSpeedCounter();
            continue;
        }
        if (actor->health <= 0) continue; // in case the actor died from an on-tick effect
        if (actor->isPlayer) {
            ++turnCount;
            clearDeadActors();
            return; // skip player
        }

        actor->advanceSpeedCounter();
        const MapTile *tile = at(actor->position);
        if (tile && tile->isSeen) {
            double dist = actor->position.distanceTo(world.player->position);
            if (dist < 2) {
                AttackData attackData = actor->meleeAttack(world.player);
                world.addMessage(buildCombatMessage(actor, world.player, attackData, configData.getBoolValue("show_combat_math", false)));
            } else {
                actor->playerLastSeenPosition = world.player->position;
                Direction dirToPlayer = actor->position.directionTo(world.player->position);
                tryActorStepApprox(actor, dirToPlayer);
            }
        } else {
            Coord oldPos = actor->position;
            if (actor->playerLastSeenPosition.x >= 0) {
                Direction dirToPlayer = actor->position.directionTo(actor->playerLastSeenPosition);
                bool result = tryActorStepApprox(actor, dirToPlayer);
                if (!result || actor->position == actor->playerLastSeenPosition) actor->playerLastSeenPosition.x = -1;
            } else if (globalRNG.upto(2)) {
                Direction dir = randomDirection();
                tryActorStepApprox(actor, dir);
            }
            Coord newPos = actor->position;
            if (oldPos != newPos && (isSeen(oldPos) || isSeen(newPos))) return;
        }
    }
}

void Dungeon::clearFovCalc() {
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            MapTile *tile = at(Coord(x, y));
            if (tile) tile->inFovCalc = false;
        }
    }
    return;
}

std::vector<Coord> Dungeon::getEffectArea(const Coord &origin, const Coord &target, int areaType, int maxRange, bool includeWalls, bool includeOrigin) {
    std::vector<Coord> result;
    if (areaType == AR_PASSIVE || areaType == AR_NONE) return result;
    switch(areaType) {
        case AR_CONE: {
            Direction areaDirection = origin.directionTo(target);
            clearFovCalc();
            fovCalcBeam(this, origin, areaDirection, maxRange);
            break; }
        case AR_BURST:
            clearFovCalc();
            fovCalcBurst(this, origin, maxRange);
            break;
        case AR_LINE:
            result = calcLine(*this, origin, target, true, true);
            for (auto iter = result.begin(); iter != result.end(); ) {
                if (!isValidPosition(*iter)) iter = result.erase(iter);
                else if (!includeOrigin && *iter == origin) iter = result.erase(iter);
                else {
                    const MapTile *tile = at(*iter);
                    if (getTileData(tile->floor).isOpaque) iter = result.erase(iter);
                    else ++iter;
                }
            }
            break;
        default:
            logMessage(LOG_ERROR, "Unhandled areaType in getEffectArea");
    }

    if (areaType == AR_CONE || areaType == AR_BURST) {
        for (int y = 0; y < mHeight; ++y) {
            for (int x = 0; x < mWidth; ++x) {
                Coord here(x, y);
                const MapTile *tile = at(here);
                if (tile && tile->inFovCalc) {
                    if (!includeOrigin && here == origin) continue;
                    if (!includeWalls && getTileData(tile->floor).isOpaque) continue;
                    result.push_back(here);
                }
            }
        }
    }
    return result;
}

bool Dungeon::inOverlay(const Coord &where) const {
    for (const Coord &pos : overlayTiles) {
        if (pos == where) return true;
    }
    return false;
}

void Dungeon::activateAbility(World &world, unsigned ident, const Coord &cursorPos, const std::vector<Coord> &targetArea) {
    const AbilityData &data = getAbilityData(ident);
    if (data.ident == BAD_VALUE) {
        world.addMessage("Tried to use invalid ability");
        return;
    }
    if (world.player->energy < data.energyCost) {
        world.addMessage("You don't have enough energy to use your " + data.name + ".");
        return;
    }
    world.player->energy -= data.energyCost;

    std::vector<Actor*> targets;
    std::string message = "[color=yellow]You[/color] use your [color=yellow]" + data.name + "[/color]. ";
    if (data.areaType == AR_NONE) {
        for (const EffectData &effect : data.effects) {
            message += triggerEffect(effect, world.player, world.player);
        }
    } else {
        if (!data.noEffectAnim) {
            overlayTiles = targetArea;
            overlayR = data.effectR;
            overlayG = data.effectG;
            overlayB = data.effectB;
            overlayGlyph = data.effectGlyph;
        }
        for (const Coord &pos : targetArea) {
            Actor *actor = actorAt(pos);
            if (!actor) continue;

            for (const EffectData &effect : data.effects) {
                message += triggerEffect(effect, world.player, actor);
            }
        }
    }
    world.addMessage(message);
}

bool Dungeon::loadMapFromFile(const std::string &filename) {
    std::string mapFileStr = readFile(filename);
    if (mapFileStr.empty()) return false;

    // clear any existing map data
    unsigned mapDataSize = mWidth * mHeight;
    for (Actor *actor : mActors) delete actor;
    mActors.clear();
    mRooms.clear();
    for (unsigned i = 0; i < mapDataSize; ++i) {
        for (Item *item : mData[i].items) delete item;
        mData[i].items.clear();
        mData[i].actor = nullptr;
        mData[i].floor = TILE_UNASSIGNED;
    }

    std::map<int, int> tileMap{
        { '#', TILE_WALL },
        { '.', TILE_FLOOR },
        { ',', TILE_GRASS },
        { '>', TILE_STAIR_DOWN },
        { '~', TILE_WATER },
        { '%', TILE_NOTHING },
        { '/', TILE_OPEN_DOOR },
        { '+', TILE_CLOSED_DOOR },
    };

    // load the map from the file
    std::stringstream mapFile(mapFileStr);
    std::string line;
    unsigned mapPos = 0;
    while (std::getline(mapFile, line)) {
        if (line.empty()) continue; // skip blank lines
        if (line[0] == '@') {
            // process directive
            // none are actually implemented, but this is where they'll go
        } else {
            for (char c : line) {
                if (c == '\n' || c == '\r' || c == '\t') continue;
                if (mapPos >= mapDataSize) {
                    logMessage(LOG_ERROR, filename + " contains excess map data");
                    break;
                }
                auto iter = tileMap.find(c);
                if (iter == tileMap.end()) {
                    logMessage(LOG_ERROR, filename + " has unrecognized map symbol " + c);
                } else {
                    mData[mapPos].floor = iter->second;
                    ++mapPos;
                }
            }
        }
    }

    if (mapPos < mapDataSize) {
        logMessage(LOG_ERROR, filename + " contains insufficent map data");
    }

    return true;
}

std::string makeItemList(const std::vector<Item*> &itemList, unsigned maxList) {
    if (itemList.empty()) return "nothing";
    if (itemList.size() == 1) return itemList.front()->getName();
    if (itemList.size() == 2) {
        std::stringstream s;
        s << itemList[0]->getName() << " and " << itemList[1]->getName();
        return s.str();
    }
    if (itemList.size() > maxList) {
        return "a large number of items";
    }
    std::stringstream s;
    for (unsigned i = 0; i < itemList.size(); ++i) {
        const Item *item = itemList[i];
        if (!item) continue;
        if (i > 0) s << ",";
        if (i == itemList.size() - 1) s << " and";
        if (i != 0) s << ' ';
        s << item->getName();
    }
    return s.str();
}
