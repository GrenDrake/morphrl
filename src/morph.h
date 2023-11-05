#ifndef MORPH_H
#define MORPH_H

#include <iosfwd>
#include <string>
#include <vector>


class Item;
class World;


const int MAP_WIDTH = 63;
const int MAP_HEIGHT = 31;

const int RT_GENERIC = 0;
const int RT_ENTRANCE = 1;
const int RT_STAIR = 1;

const int TILE_UNASSIGNED = 0;
const int TILE_WALL = 1;
const int TILE_FLOOR = 2;
const int TILE_MARKER = 3;
const int TILE_DOOR = 4;
const int TILE_WATER = 5;
const int TILE_STAIR_DOWN = 6;
const int TILE_STAIR_UP = 7;

const int ET_BOOST = 0;         // provides a constant, static boost to an actor's stats
                                // effectId = statNumber, effectStrength = amount of boost
const int ET_GIVE_ABILITY = 1;  // grants a specific ability as long as the source is equipped this ability may
                                // be triggered by the actor as their action for the turn
                                // effectId = ability to give
const int ET_ON_HIT = 2;        // triggers a special effect every time the actor makes a successful attack
const int ET_ON_USE = 3;        // triggers a special effect when the item is used from the inventory (item may be destroyed)
const int ET_ON_TICK = 4;       // triggers a special effect at the end of the actor's turn, every turn


const int STAT_STRENGTH     = 0;
const int STAT_AGILITY      = 1;
const int STAT_DEXTERITY    = 2;
const int STAT_TOUGHNESS    = 3;
const int STAT_BASE_COUNT   = 4;
const int STAT_TO_HIT       = 4;
const int STAT_EVASION      = 5;
const int STAT_XP           = 6;
const int STAT_EXTRA_COUNT  = 7;
const int STAT_BULK         = 7;
const int STAT_BULK_MAX     = 8;
const int STAT_HEALTH       = 9;
const int STAT_ENERGY       = 10;
const int STAT_ALL_COUNT    = 11;


enum class Direction {
    Unknown, Here,
    North, Northeast, East, Southeast, South, Southwest, West, Northwest,
};

struct Color {
    int r, g, b;
};
struct Image {
    Image() : w(0), h(0), pixels(nullptr) {}
    ~Image();
    int w, h;
    unsigned char *pixels;

    Color at(int x, int y) const;
};

struct Coord {
    Coord() : x(0), y(0) { }
    Coord(int x, int y) : x(x), y(y) { }

    bool operator==(const Coord &rhs) const;
    Coord shift(Direction d, int amount = 1) const;
    Direction directionTo(const Coord &to) const;
    double distanceTo(const Coord &to) const;

    int x, y;
};

struct EffectData {
    int trigger;        // BOOST, GIVE_ABILITY, ON_HIT, ON_USE, ON_TICK
    int effectChance;
    int effectId;       // internal ID mapped to exact effect
    int effectStrength;
};

struct ActorData {
    unsigned ident;
    int glyph;
    int r, g, b;
    std::string name, desc;
    int baseStats[STAT_BASE_COUNT];
};
struct ItemData {
    enum Type {
        Invalid, 
        Junk, 
        //              arg1        arg2        arg3
        Weapon,      // attackBonus minDamage   maxDamage
        Talisman, 
        Consumable
    };

    unsigned ident;
    int glyph;
    int r, g, b;
    std::string name, desc;
    Type type;
    int bulk;
    int toHit, minDamage, maxDamage;
    int consumeChance;
    int statMods[STAT_BASE_COUNT];
    std::vector<EffectData> effects;
};
struct TileData {
    unsigned ident;
    int glyph;
    std::string name, desc;
    bool isPassable;
    bool isOpaque;
    int r, g, b;
    bool isUpStair;
    bool isDownStair;
};


struct AttackResult {
    bool didHit;
    int roll, target, damage;
};
struct Actor {
    Actor(const ActorData &data, unsigned myIdent);

    bool isDead() const { return health <= 0; }

    std::string getName(bool definitive = false) const;
    void reset();
    int getStat(int statNumber) const;
    // AttackResult makeAttack(Actor *target);
    void takeDamage(int amount);
    void spendEnergy(int amount);

    bool isOverBurdened() const;
    void addItem(Item *item);
    void removeItem(Item *item);
    int getTalismanCount() const;
    const Item* getCurrentWeapon() const;

    const ActorData &data;
    unsigned ident;
    Coord position;
    bool isPlayer;
    int xp;
    Coord playerLastSeenPosition;

    int health, energy;
    std::vector<Item*> inventory;
};

struct Item {
    Item(const ItemData &data);

    std::string getName(bool definitive = false) const;
    int getStatBonus(int statNumber) const;

    const ItemData &data;
    Coord position;
    bool isEquipped;
};

struct MapTile {
    MapTile();
    int floor;
    Actor *actor;
    Item *item;
    int temperature;
    bool isSeen, everSeen;
    int distanceValue;
};


struct Room {
    int type;
    int x, y, w, h;
    Direction roomDirection;
    bool isFilled;
    
    int area() const;
    Coord getPointWithin() const;
};


class Dungeon {
public:
    Dungeon(int depth, int width, int height);

    int depth() const { return mDepth; };
    int width() const { return mWidth; };
    int height() const { return mHeight; };

    void clear();
    void clearDistances();
    bool areaIsTile(int x, int y, int w, int h, int theTile) const;
    void drawRect(int x, int y, int w, int h, int toTile);
    void fillRect(int x, int y, int w, int h, int toTile);
    void replaceTile(int fromTile, int toTile);
    void calcDistances(const Coord &fromWhere);
    Coord nearestOpenTile(const Coord &source, bool allowActor=true, bool allowItem=false) const;
    Coord randomOpenTile(bool allowActor=false, bool allowItem=true) const;
    Coord randomOfTile(int theTile) const;

    bool isValidPosition(const Coord &where) const;
    unsigned toPosition(const Coord &where) const;

    int floorAt(const Coord &where) const;
    void floorAt(const Coord &where, int toTile);
    const MapTile* at(const Coord &where) const;
    MapTile* at(const Coord &where);

    bool isSeen(const Coord &where) const;
    bool everSeen(const Coord &where) const;
    void setSeen(const Coord &where);
    void clearIsSeen();
    void doActorFOV(Actor *actor);

    bool addActor(Actor *who, const Coord &where);
    bool moveActor(Actor *who, const Coord &where);
    bool tryActorStep(Actor *who, Direction dir);
    bool tryActorStepApprox(Actor *who, Direction dir);
    bool removeActor(Actor *who);
    const Actor* actorAt(const Coord &where) const;
    Actor* actorAt(const Coord &where);

    bool addItem(Item *what, const Coord &where);
    bool removeItem(Item *what);
    const Item* itemAt(const Coord &where) const;
    Item* itemAt(const Coord &where);

    void addRoom(const Room &room);
    int roomCount() const { return mRooms.size(); }
    const Room& getRoomByType(int roomType) const;
    // const Room& getRoom(int index) const;
    Room& getRoom(int index);

    void clearDeadActors();
    void tick(World &world);

private:
    int mDepth;
    int mWidth, mHeight;
    std::vector<Room> mRooms;
    std::vector<Actor*> mActors;
    MapTile *mData;
};


struct LogMessage {
    std::string text;
};

class World {
public:
    World();
    ~World();
    Actor *player;
    Dungeon *map;
    unsigned currentTurn;
    std::vector<LogMessage> messages;
    bool disableFOV;

    void addMessage(const std::string &text);

    void tick();
};

int percentOf(int percent, int ofValue);
std::string ucFirst(std::string text);
std::vector<std::string> explode(const std::string &text, char onChar);
std::vector<std::string> explodeOnWhitespace(std::string text);
const std::string& trim(const std::string &text);
std::string& trim(std::string &text);
std::string intToString(long long number);
// int strToInt(const std::string &text);
bool strToInt(const std::string &text, int &result);

Image* loadImage(const std::string &filename);
void drawImage(int originX, int originY, Image *image);

Direction randomDirection();
Direction randomCardinalDirection();
Direction rotate45(Direction d);
Direction unrotate45(Direction d);
Direction rotate90(Direction d);

std::string statName(int statNumber);

std::ostream& operator<<(std::ostream &out, Direction d);
std::ostream& operator<<(std::ostream &out, const Coord &where);

bool loadAllData();
const ActorData& getActorData(unsigned ident);
const ItemData& getItemData(unsigned ident);
const TileData& getTileData(unsigned ident);

void handlePlayerFOV(Dungeon *dungeon, Actor *player);
void doMapgen(Dungeon &d);

#endif // MORPH_H
