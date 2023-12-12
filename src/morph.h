#ifndef MORPH_H
#define MORPH_H

#include <iosfwd>
#include <string>
#include <vector>

#include "random.h"

class Actor;
class Dungeon;
class Item;
class World;


const unsigned BAD_VALUE = 4294967295;

const int LOG_INFO = 0;
const int LOG_WARN = 1;
const int LOG_ERROR = 2;
const int LOG_DEBUG = 3;

const int MAP_WIDTH = 63;
const int MAP_HEIGHT = 47;
const int MAX_TALISMANS_WORN = 3;

const int DE_ENTRANCE = 0;
const int DE_UPSTAIRS = 1;
const int DE_DOWNSTAIRS = 2;

const int RT_GENERIC = 0;
const int RT_ENTRANCE = 1;
const int RT_STAIR = 2;

const int TILE_UNASSIGNED = 0;
const int TILE_WALL = 1;
const int TILE_FLOOR = 2;
const int TILE_MARKER = 3;
const int TILE_CLOSED_DOOR = 4;
const int TILE_WATER = 5;
const int TILE_STAIR_DOWN = 6;
const int TILE_STAIR_UP = 7;
const int TILE_GRASS = 8;
const int TILE_OPEN_DOOR = 9;
const int TILE_NOTHING = 10;

const int AR_NONE = 0;
const int AR_TARGET = 1;
const int AR_CONE = 2;
const int AR_LINE = 3;
const int AR_BURST = 4;
const int AR_PASSIVE = 5;

const int ET_BOOST = 0;         // provides a constant, static boost to an actor's stats
                                // effectId = statNumber, effectStrength = amount of boost
const int ET_GIVE_ABILITY = 1;  // grants a specific ability as long as the source is equipped this ability may
                                // be triggered by the actor as their action for the turn
                                // effectId = ability to give
const int ET_ON_HIT = 2;        // triggers a special effect every time the actor makes a successful attack
const int ET_ON_USE = 3;        // triggers a special effect when the item is used from the inventory (item may be destroyed)
const int ET_ON_TICK = 4;       // triggers a special effect at the end of the actor's turn, every turn
const int ET_UNARMED_ATTACK = 5;

const unsigned STATUS_UNLIMITED_DURATION = 4294967295;

const int STAT_STRENGTH     = 0;
const int STAT_AGILITY      = 1;
const int STAT_SPEED        = 2;
const int STAT_TOUGHNESS    = 3;
const int STAT_BASE_COUNT   = 4;
const int STAT_TO_HIT       = 4;
const int STAT_EVASION      = 5;
const int STAT_BULK_MAX     = 6;
const int STAT_EXTRA_COUNT  = 7;
const int STAT_HEALTH       = 7;
const int STAT_ENERGY       = 8;
const int STAT_DAMAGE_BONUS = 9;
const int STAT_BULK         = 10;
const int STAT_ALL_COUNT    = 11;

const int TT_ITEM = 0;
const int TT_MUTATION = 1;
const int TT_STATUS_EFFECT = 2;

const int EFFECT_HEALING    = 0; // instant healing - strength = percent healed
const int EFFECT_DAMAGE     = 1; // take damage - strength = amount
const int EFFECT_MUTATE     = 2; // add mutation - strength = # to add
const int EFFECT_PURIFY     = 3; // purify mutation - strength = # to remove
const int EFFECT_ADJ_ENERGY = 4;
const int EFFECT_ATTACK     = 5;
const int EFFECT_APPLY_STATUS = 6;

const int XP_PER_LEVEL      = 100;

// special item numbers
const int SIN_FISTS         = -2;

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
    bool operator!=(const Coord &rhs) const;
    Coord shift(Direction d, int amount = 1) const;
    Direction directionTo(const Coord &to) const;
    double distanceTo(const Coord &to) const;
    std::string toString() const;

    int x, y;
};

struct Origin {
    Origin();
    Origin(const std::string &filename, unsigned lineNumber);
    std::string toString() const;

    std::string filename;
    unsigned lineNumber;
};

struct ConfigValue {
    std::string name;
    std::string value;
    bool isInt;
    int asInt;
};
struct ConfigData {
    std::vector<ConfigValue> values;

    const ConfigValue& getRawValue(const std::string &name) const;
    bool hasValue(const std::string &name) const;
    bool isInt(const std::string &name) const;
    const std::string& getValue(const std::string &name, const std::string &defaultValue = "") const;
    int getIntValue(const std::string &name, int defaultValue = -1) const;
    bool getBoolValue(const std::string &name, bool defaultValue = false) const;
};

struct EffectData {
    int trigger;        // BOOST, GIVE_ABILITY, ON_HIT, ON_USE, ON_TICK
    int effectChance;
    int effectId;       // internal ID mapped to exact effect
    int effectStrength;
    int effectParam;

    std::string toString() const;
};

struct StatusData {
    unsigned ident;
    std::string name;
    std::string desc;
    unsigned minDuration;        // the status effect should last at least this long
    unsigned maxDuration;        // the status effect should never last longer than
                            // this (use a high value (4294967295) to allow
                            // "permenant" effects
    int resistDC;           // the DC required to resist the effect
    bool resistEveryTurn;   // should the actor retry the resistance until cured?
    std::vector<EffectData> effects;
};

struct MutationData {
    unsigned ident;
    std::string gainVerb;
    std::string name;
    std::string desc;
    unsigned slot;          // what part of the body is mutated? (arms, tail,
                            // etc.) or 0 for "minor" muations that do not require a slot
    std::vector<EffectData> effects;
};


struct AbilityData {
    unsigned ident;
    std::string name, desc;
    int energyCost;
    int areaType;
    int maxRange;
    int speedMult;
    int effectR, effectG, effectB;
    int effectGlyph;
    bool noEffectAnim;
    std::vector<EffectData> effects;
};


struct SpawnLine {
    int spawnGroup;
    int spawnChance;
    int ident;
};

struct ActorData {
    unsigned ident;
    int glyph;
    int r, g, b;
    std::string name, desc;
    std::string artFile;
    int baseLevel;
    int baseStats[STAT_BASE_COUNT];
    std::vector<SpawnLine> initialItems;
    std::vector<SpawnLine> initialMutations;
};

struct ItemData {
    enum Type {
        Invalid,
        Junk,
        Weapon,
        Talisman,
        Consumable
    };

    unsigned ident;
    int glyph;
    int r, g, b;
    std::string name, desc;
    Type type;
    int bulk;
    int minDamage, maxDamage;
    int consumeChance;
    bool isVictoryArtifact;
    std::vector<EffectData> effects;
};

struct TileData {
    unsigned ident;
    int glyph;
    std::string name;
    unsigned interactTo;
    bool isPassable;
    bool isOpaque;
    int r, g, b;
    bool isUpStair;
    bool isDownStair;
};

struct DungeonData {
    unsigned ident; // doubles as dungeon depth
    std::string name;
    bool hasUpStairs;
    bool hasDownStairs;
    bool fromFile;
    unsigned actorCount;
    unsigned itemCount;
    Coord initialPosition;
    std::vector<SpawnLine> actorSpawns;
    std::vector<SpawnLine> itemSpawns;
};


struct AttackData {
    int roll;
    int toHit;
    int evasion;
    int damage;
    int damageMin, damageMax, damageBonus;
    const Item *weapon;
    std::vector<Item*> drops;
    std::string effectsMessage;
    std::string errorMessage;
};

struct StatusItem {
    StatusItem(const StatusData &data);

    const StatusData &data;
    Actor *fromWho;
    unsigned duration;      // the number of times this status effect has triggered
};

struct MutationItem {
    MutationItem(const MutationData &data);

    const MutationData &data;
};

struct Actor {
    static Actor* create(const ActorData &data);
    ~Actor();

    bool isDead() const { return health <= 0; }

    std::string getName(bool definitive = false) const;
    void reset();
    void verify();
    int getStatLevelBonus(int statNumber) const;
    int getStatStatusBonus(int statNumber) const;
    int getStatMutationBonus(int statNumber) const;
    int getStatItemBonus(int statNumber) const;
    int getStatBase(int statNumber) const;
    int getStat(int statNumber) const;
    // AttackResult makeAttack(Actor *target);
    void takeDamage(int amount, Actor *fromWho);
    void spendEnergy(int amount);
    void giveXP(int amount);

    bool isOverBurdened() const;
    void addItem(Item *item);
    void removeItem(Item *item);
    void dropAllItems();
    bool tryEquipItem(Item *item);
    int getTalismanCount() const;
    bool hasVictoryArtifact() const;
    const Item* getCurrentWeapon() const;
    std::string triggerOnHitEffects(Actor *target);
    AttackData meleeAttackWithWeapon(Actor *target, const Item *weapon);
    AttackData meleeAttack(Actor *target);
    void advanceSpeedCounter(int multiplier = 100);
    MutationItem* mutationForSlot(unsigned slotNumber);
    bool hasMutation(unsigned mutationIdent) const;
    void applyMutation(MutationItem *mutation);
    void removeMutation(MutationItem *mutation);
    void applyStatus(StatusItem *statusItem);
    bool hasStatus(unsigned statusIdent) const;
    std::vector<unsigned> getAbilityList() const;

    const ActorData &data;
    int statLevels[STAT_BASE_COUNT];
    unsigned ident;
    Coord position;
    bool isPlayer;
    int level;
    int xp, advancementPoints;
    Coord playerLastSeenPosition;
    unsigned speedCounter;

    int health, energy;
    std::vector<Item*> inventory;
    std::vector<StatusItem*> statusEffects;
    std::vector<MutationItem*> mutations;
    Dungeon *onMap;

private:
    Actor(const ActorData &data, unsigned myIdent);
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
    std::vector<Item*> items;
    int temperature;
    bool isSeen, everSeen, inFovCalc;
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
    Dungeon(const DungeonData &data, int width, int height);

    int depth() const { return mDepth; };
    int width() const { return mWidth; };
    int height() const { return mHeight; };

    bool loadMapFromFile(const std::string &filename);

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
    Coord firstOfTile(int theTile) const;

    bool isValidPosition(const Coord &where) const;
    unsigned toPosition(const Coord &where) const;

    int distanceAt(const Coord &where) const;
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
    void resetSpeedCounter();

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
    Actor* getNextActor();
    unsigned getHighestSpeedCounter() const;
    void tick(World &world);

    void clearFovCalc();
    std::vector<Coord> getEffectArea(const Coord &origin, const Coord &target, int areaType, int maxRange, bool includeWalls, bool includeOrigin);
    bool inOverlay(const Coord &where) const;
    void activateAbility(World &world, unsigned ident, const Coord &cursorPos, const std::vector<Coord> &targetArea);

    std::vector<Coord> overlayTiles;
    int overlayR, overlayG, overlayB;
    int overlayGlyph;
    const DungeonData &data;
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

enum class GameState {
    Normal, Victory
};
class World {
public:
    World();
    ~World();

    Actor *player;
    Dungeon *map;
    unsigned currentTurn;
    std::vector<LogMessage> messages;
    std::vector<Dungeon*> levels;
    bool disableFOV;
    uint64_t gameSeed;
    bool showCombatMath;
    GameState gameState;

    Dungeon* getDungeon(int depth);
    bool movePlayerToDepth(int newDepth, int enterFrom);
    void addMessage(const std::string &text);

    void tick();
};


struct DocumentImage {
    std::string filename;
    int x, y, w, h;
    Image *image;
};

struct Document {
    ~Document();
    std::vector<std::string> lines;
    std::vector<DocumentImage> images;
};

template<class T>
bool vectorContains(std::vector<T> &v, T item) {
    for (const T &iter : v) {
        if (iter == item) return true;
    }
    return false;
}


int percentOf(int percent, int ofValue);
std::string ucFirst(std::string text);
std::vector<std::string> explode(const std::string &text, char onChar);
std::vector<std::string> explodeOnWhitespace(std::string text);
const std::string& trim(const std::string &text);
std::string& trim(std::string &text);
std::string intToString(long long number);
bool strToInt(const std::string &text, unsigned &result);
bool strToInt(const std::string &text, int &result);
void addUniqueToVector(std::vector<int> &v, int item);

void showDocument(const std::string &filename);
void showDocument(Document *document);
Document* loadDocument(const std::string &filename);

Image* loadImage(const std::string &filename);
void drawImage(int originX, int originY, Image *image);

Direction randomDirection();
Direction randomCardinalDirection();
Direction rotate45(Direction d);
Direction unrotate45(Direction d);
Direction rotate90(Direction d);

std::string makeItemList(const std::vector<Item*> &itemList, unsigned maxList);
std::string statName(int statNumber);

std::ostream& operator<<(std::ostream &out, Direction d);
std::ostream& operator<<(std::ostream &out, const Coord &where);

bool loadAllData();
std::string readFile(const std::string &filename);
std::vector<unsigned char> readFileAsBinary(const std::string &filename);
const ActorData& getActorData(unsigned ident);
const ItemData& getItemData(unsigned ident);
const MutationData& getRandomMutationData();
const AbilityData& getAbilityData(unsigned ident);
const MutationData& getMutationData(unsigned ident);
const StatusData& getStatusData(unsigned ident);
const TileData& getTileData(unsigned ident);
unsigned getDungeonEntranceIdent();
const DungeonData& getDungeonData(unsigned ident);

std::string buildCombatMessage(Actor *attacker, Actor *victim, const AttackData &attackData, bool showCalc);
std::string triggerEffect(const EffectData &effect, Actor *user, Actor *target);
void handlePlayerFOV(Dungeon *dungeon, Actor *player);
void fovCalcBeam(Dungeon *dungeon, const Coord &origin, Direction dir, int maxRange);
void fovCalcBurst(Dungeon *dungeon, const Coord &origin, int maxRange);
std::vector<Coord> calcLine(const Dungeon &map, const Coord &start, const Coord &end, bool stopOpaque, bool stopSolid);
void doMapgen(Dungeon &d);
void spawnActors(Dungeon &d, bool forRefresh);
void activateItem(World &world, Item *item, Actor *user);

void ui_alertBox(const std::string &title, const std::string &message);
bool ui_getString(const std::string &title, const std::string &message, std::string &result);

void logMessage(int logLevel, std::string message);
bool loadConfigData(const std::string &filename);

extern ConfigData configData;
extern RNG globalRNG;

#endif // MORPH_H
