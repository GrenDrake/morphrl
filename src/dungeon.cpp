#include <deque>
#include <iostream>
#include <vector>

#include "morph.h"



Actor::Actor(const ActorData &data, unsigned myIdent)
: data(data), ident(myIdent), position(-1, -1),
  isPlayer(false), xp(0), playerLastSeenPosition(-1, -1)
{ }

std::string Actor::getName(bool definitive) const {
    if (isPlayer) return "you";
    if (definitive) return "the " + data.name;
    else            return "a " + data.name;
}

void Actor::reset() {
    health = getStat(STAT_HEALTH);
    energy = getStat(STAT_ENERGY);
}

int Actor::getStat(int statNumber) const {
    int itemBonus = 0;
    for (const Item *item : inventory) {
        if (!item->isEquipped) continue; // items only provide static bonuses when equipped
        itemBonus += item->getStatBonus(statNumber);
    }

    switch(statNumber) {
        case STAT_STRENGTH:
        case STAT_DEXTERITY:
        case STAT_AGILITY:
        case STAT_TOUGHNESS:
            return itemBonus + data.baseStats[statNumber];
        case STAT_XP:           return xp;
        case STAT_HEALTH:       return 20 + itemBonus + getStat(STAT_TOUGHNESS) * 4;
        case STAT_ENERGY:       return 20 + itemBonus + getStat(STAT_TOUGHNESS) * 2;
        case STAT_TO_HIT:       return itemBonus + getStat(STAT_AGILITY);
        case STAT_EVASION:      return 10 + itemBonus + getStat(STAT_AGILITY);
        case STAT_BULK_MAX:     return 10 + itemBonus + getStat(STAT_STRENGTH) * 2;
        case STAT_BULK: {
            int total = 0;
            for (const Item *item : inventory) {
                if (!item) continue;
                total += item->data.bulk;
            }
            return total;
            break; }
        default:
            return 0;
    }
}

void Actor::takeDamage(int amount) {
    health -= amount;
    if (health < 0) health = 0;
    int maxHealth = getStat(STAT_HEALTH);
    if (health > maxHealth) health = maxHealth;
}

void Actor::spendEnergy(int amount) {
    energy -= amount;
    if (energy < 0) energy = 0;
    int maxEnergy = getStat(STAT_ENERGY);
    if (energy > maxEnergy) energy = maxEnergy;
}

bool Actor::isOverBurdened() const {
    return getStat(STAT_BULK) > getStat(STAT_BULK_MAX);
}

void Actor::addItem(Item *item) {
    if (item) inventory.push_back(item);
}

void Actor::removeItem(Item *item) {
    if (!item) return;
    auto iter = inventory.begin();
    while (iter != inventory.end()) {
        if (*iter == item) {
            inventory.erase(iter);
            return;
        }
        ++iter;
    }
}

int Actor::getTalismanCount() const {
    int count = 0;
    for (const Item *item : inventory) {
        if (item && item->data.type == ItemData::Talisman && item->isEquipped) {
            ++count;
        }
    }
    return count;
}

const Item* Actor::getCurrentWeapon() const {
    for (const Item *item : inventory) {
        if (item && item->data.type == ItemData::Weapon && item->isEquipped) {
            return item;
        }
    }
    return nullptr;
}


Item::Item(const ItemData &data)
: data(data), position(-1, -1), isEquipped(false)
{ }

std::string Item::getName(bool definitive) const {
    if (definitive) return "the " + data.name;
    else            return "a " + data.name;
}

int Item::getStatBonus(int statNumber) const {
    // items can never provide a static bonus to current bulk (use the bulk_max
    // stat instead) or to current XP
    if (statNumber == STAT_BULK || statNumber == STAT_XP) return 0;

    int result = 0;
    for (const EffectData &effect : data.effects) {
        if (effect.trigger == ET_BOOST && effect.effectId == statNumber) {
            result += effect.effectStrength;
        }
    }

    return result;
}

MapTile::MapTile()
: floor(0), actor(nullptr), item(nullptr), temperature(0), isSeen(false), everSeen(false)
{ }


int Room::area() const {
    return (w - 2) * (h - 2);
}


Coord Room::getPointWithin() const {
    int dx = rand() % (w / 2);
    int dy = rand() % (h / 2);
    dx *= 2;
    dy *= 2;
    return Coord(1 + x + dx, 1 + y + dy);
}



Dungeon::Dungeon(int depth, int width, int height)
: mDepth(depth), mWidth(width), mHeight(height)
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
            if (!td.isPassable) continue;
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
         (allowItem || !originTile->item) &&
         (originData.isPassable)) return source;

    Direction dir = Direction::North;
    do {
        bool isGood = true;
        const MapTile *tile = at(source.shift(dir));
        if (tile) {
            const TileData &td = getTileData(tile->floor);
            if (!td.isPassable) isGood = false;
            if (!allowActor && tile->actor) isGood = false;
            if (!allowItem && tile->item) isGood = false;
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
        x = 1 + (rand() % (mWidth - 2));
        y = 1 + (rand() % (mHeight - 2));
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
        x = 1 + (rand() % (mWidth - 2));
        y = 1 + (rand() % (mHeight - 2));
        const MapTile *tile = at(Coord(x, y));
        isValid = tile && tile->floor == theTile;
    } while (iterations > 0 && !isValid);
    if (isValid) return Coord(x, y);
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


bool Dungeon::addActor(Actor *who, const Coord &where) {
    if (!who) return false;
    MapTile *tile = at(where);
    if (!tile || tile->actor) return false;
    tile->actor = who;
    who->position = where;
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


bool Dungeon::addItem(Item *what, const Coord &where) {
    if (!what) return false;
    MapTile *tile = at(where);
    if (!tile || tile->item) return false;
    tile->item = what;
    what->position = where;
    return true;
}

bool Dungeon::removeItem(Item *what) {
    if (!what) return false;
    MapTile *tile = at(what->position);
    if (!tile) return false;
    if (tile->item != what) return false;
    tile->item = nullptr;
    what->position = Coord(-1, -1);
    return true;
}

const Item* Dungeon::itemAt(const Coord &where) const {
    const MapTile *tile = at(where);
    if (!tile) return nullptr;
    return tile->item;
}
Item* Dungeon::itemAt(const Coord &where) {
    MapTile *tile = at(where);
    if (!tile) return nullptr;
    return tile->item;
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

void Dungeon::tick(World &world) {
    for (Actor *actor : mActors) {
        if (actor->isPlayer) continue; // skip player
        if (actor->health <= 0) continue; // skip dead actors

        const MapTile *tile = at(actor->position);
        if (tile && tile->isSeen) {
            double dist = actor->position.distanceTo(world.player->position);
            if (dist < 2) {
                world.addMessage("POW!");
            } else {
                actor->playerLastSeenPosition = world.player->position;
                Direction dirToPlayer = actor->position.directionTo(world.player->position);
                tryActorStepApprox(actor, dirToPlayer);
            }
        } else {
            if (actor->playerLastSeenPosition.x >= 0) {
                Direction dirToPlayer = actor->position.directionTo(actor->playerLastSeenPosition);
                bool result = tryActorStepApprox(actor, dirToPlayer);
                if (!result || actor->position == actor->playerLastSeenPosition) actor->playerLastSeenPosition.x = -1;
            } else if (rand() % 2) {
                Direction dir = randomDirection();
                tryActorStepApprox(actor, dir);
            }
        }
    }

    clearDeadActors();
}

std::string statName(int statNumber) {
    switch(statNumber) {
        case STAT_STRENGTH:     return "strength";
        case STAT_DEXTERITY:    return "dexterity";
        case STAT_AGILITY:      return "agility";
        case STAT_TOUGHNESS:    return "toughness";
        case STAT_HEALTH:       return "max health";
        case STAT_ENERGY:       return "max energy";
        case STAT_TO_HIT:       return "to hit bonus";
        case STAT_EVASION:      return "evasion";
        case STAT_BULK_MAX:     return "max bulk";
        case STAT_BULK:         return "bulk";
        case STAT_XP:           return "XP";
        default: return "unknown stat " + std::to_string(statNumber);
    }
}
