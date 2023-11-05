#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "morph.h"

const unsigned BAD_VALUE = 4294967295;

std::vector<ActorData> actorData;
    // { 0, '@', 127, 255, 127, "player", { 10, 10, 10, 10 } },
    // { 1, 'g', 255, 127, 127, "goblin", {  8,  8,  8,  8 } },

std::vector<ItemData> itemData;
    // { 0, '/', 127, 127, 127, "sword",  ItemData::Weapon, 2, 2, 7, 13, { 0, 0, 0, 0 } },
    // { 1, '/', 127, 127, 127, "dagger", ItemData::Weapon, 3, 4,  3, 7, { 0, 0, 0, 0 } },
    // { 2, '!', 255,   0,   0, "potion of healing", ItemData::Consumable, 1, 0,  0, 0, { 0, 0, 0, 0 } },
    // { 3, '!', 255, 127, 127, "potion of flames", ItemData::Consumable, 1, 0,  0, 0, { 0, 0, 0, 0 } },
    // { 4, '"', 127, 127, 127, "talisman of armour", ItemData::Talisman, 1, 0,  0, 0, { 0, 0, 0, 0 } },
    // { 5, '"', 127, 127, 127, "talisman of health", ItemData::Talisman, 1, 0,  0, 0, { 0, 0, 0, 0 } },
    // { 6, '%', 127, 127, 127, "brick", ItemData::Junk, 2, 0, 0, 0, { 0, 0, 0, 0 } },


std::vector<TileData> tileData;
    // { 0, '?', "unassigned", false, true,  255, 0,   255 },
    // { 1, '#', "wall",       false, true,  127, 127, 127 },
    // { 2, '.', "floor",      true,  false, 255, 255, 255 },
    // { 3, '*', "marker",     true,  false, 255, 0,   255 },
    // { 4, '+', "door",       true,  true,  165, 42,  42  },
    // { 5, '~', "water",      false, false, 127, 196, 127 },


ActorData BAD_ACTOR{BAD_VALUE, '?', 255, 0, 255, "invalid"};
const ActorData& getActorData(unsigned ident) {
    for (const ActorData &data : actorData) {
        if (data.ident == ident) return data;
    }
    return BAD_ACTOR;
}

ItemData BAD_ITEM{BAD_VALUE, '?', 255, 0, 255, "invalid", "", ItemData::Invalid};
const ItemData& getItemData(unsigned ident) {
    for (const ItemData &data : itemData) {
        if (data.ident == ident) return data;
    }
    return BAD_ITEM;
}

TileData BAD_TILE{BAD_VALUE, '?', "invalid"};
const TileData& getTileData(unsigned ident) {
    for (const TileData &data : tileData) {
        if (data.ident == ident) return data;
    }
    return BAD_TILE;
}



std::string convertUnderscores(std::string text) {
    for (char &c : text) {
        if (c == '_') c = ' ';
    }
    return text;
}

struct DataDef {
    std::string name;
    unsigned partCount;
};
std::vector<DataDef> dataDefs{
    { "@tile", 3 },
    { "@actor", 3 },
    { "@item", 3 },
    { ";", 1 },

    { "glyph", 2 },
    { "name", 2 },
    { "description", 2 },
    { "colour", 4 },
    { "effect", 5 },

    { "base_strength", 2 },
    { "base_agility", 2 },
    { "base_dexterity", 2 },
    { "base_toughness", 2 },

    { "type", 2 },
    { "bulk", 2 },
    { "to_hit", 2 },
    { "damage", 3 },
    { "consumeChance", 2 },

    { "isOpaque", 1 },
    { "isPassable", 1 },
    { "isUpStair", 1 },
    { "isDownStair", 1 },
};
const DataDef BAD_DEF{ "INVALID", BAD_VALUE };
const DataDef& getDataDef(const std::string &text) {
    for (const DataDef &def : dataDefs) {
        if (def.name == text) return def;
    }
    return BAD_DEF;
}
bool loadAllData() {
    const std::string &filename = "resources/game.dat";
    std::ifstream inf(filename);
    if (!inf) {
        std::cerr << "Failed to load game data from " << filename << ".\n";
        return false;
    }

    // Item *item = nullptr;
    ActorData actor;
    ItemData item;
    TileData tile;
    const int TYPE_NONE  = 0;
    const int TYPE_ACTOR = 1;
    const int TYPE_ITEM  = 2;
    const int TYPE_TILE  = 3;
    int currentType = TYPE_NONE;

    int errorCount = 0;
    int lineNumber = 0;
    std::string line;
    while (std::getline(inf, line)) {
        ++lineNumber;
        auto parts = explodeOnWhitespace(line);
        if (parts.empty() || parts[0][0] == '#') continue;

        const DataDef &def = getDataDef(parts[0]);
        if (def.partCount == BAD_VALUE) {
            std::cerr << filename << ':' << lineNumber;
            std::cerr << "  unknown command or attribute " << parts[0] << ".\n";
            ++errorCount;
            continue;
        }
        if (def.partCount != parts.size()) {
            std::cerr << filename << ':' << lineNumber;
            std::cerr << "  bad argument count for command or attribute ";
            std::cerr << parts[0] << " (expected " << def.partCount << ", but found ";
            std::cerr << parts.size() << ").\n";
            ++errorCount;
            continue;
        }

        if (parts[0] == ";") {
            switch(currentType) {
                case TYPE_ACTOR:
                    actorData.push_back(actor);
                    break;
                case TYPE_ITEM:
                    itemData.push_back(item);
                    break;
                case TYPE_TILE:
                    tileData.push_back(tile);
                    break;
                case TYPE_NONE:
                    std::cerr << filename << ':' << lineNumber;
                    std::cerr << "  encountered ; outside of def\n";
                    ++errorCount;
                    break;
                default:
                    std::cerr << filename << ':' << lineNumber;
                    std::cerr << "  encountered ; in unhandled data type\n";
                    ++errorCount;
            }
            currentType = TYPE_NONE;

        // ACTOR SPECIFIC
        } else if (parts[0] == "@actor") {
            if (currentType != TYPE_NONE) {
                std::cerr << filename << ':' << lineNumber;
                std::cerr << "  tried to start actor def while already in def\n";
                ++errorCount;
                continue;
            }
            currentType = TYPE_ACTOR;
            unsigned ident = strToInt(parts[1]);
            actor.ident = ident;
            actor.name = "unknown";
            actor.desc = "";
            actor.glyph = '?';
            actor.r = 255; actor.g = 0; actor.b = 255;
            for (int i = 0; i < STAT_BASE_COUNT; ++i) {
                actor.baseStats[STAT_BASE_COUNT] = 1;
            }
        } else if (parts[0] == "base_strength") {
            actor.baseStats[STAT_STRENGTH] = strToInt(parts[1]);
        } else if (parts[0] == "base_agility") {
            actor.baseStats[STAT_AGILITY] = strToInt(parts[1]);
        } else if (parts[0] == "base_dexterity") {
            actor.baseStats[STAT_DEXTERITY] = strToInt(parts[1]);
        } else if (parts[0] == "base_toughness") {
            actor.baseStats[STAT_TOUGHNESS] = strToInt(parts[1]);

        // ITEM SPECIFIC
        } else if (parts[0] == "@item") {
            if (currentType != TYPE_NONE) {
                std::cerr << filename << ':' << lineNumber;
                std::cerr << "  tried to start item def while already in def\n";
                ++errorCount;
                continue;
            }
            currentType = TYPE_ITEM;
            unsigned ident = strToInt(parts[1]);
            item.ident = ident;
            item.name = "unknown";
            item.desc = "";
            item.glyph = '?';
            item.r = 255; actor.g = 0; actor.b = 255;
            item.type = ItemData::Junk;
            item.bulk = 1;
            item.toHit = 0;
            item.minDamage = 0;
            item.maxDamage = 0;
            item.consumeChance = 0;
            item.effects.clear();
            for (int i = 0; i < STAT_BASE_COUNT; ++i) {
                item.statMods[STAT_BASE_COUNT] = 1;
            }
        } else if (parts[0] == "bulk") {
            item.bulk = strToInt(parts[1]);
        } else if (parts[0] == "to_hit") {
            item.toHit = strToInt(parts[1]);
        } else if (parts[0] == "damage") {
            item.minDamage = strToInt(parts[1]);
            item.maxDamage = strToInt(parts[2]);
        } else if (parts[0] == "consumeChance") {
            item.consumeChance = strToInt(parts[1]);
        } else if (parts[0] == "type") {
            if (parts[1] == "weapon") item.type = ItemData::Weapon;
            else if (parts[1] == "consumable") item.type = ItemData::Consumable;
            else if (parts[1] == "talisman") item.type = ItemData::Talisman;
            else if (parts[1] == "junk") item.type = ItemData::Junk;
            else {
                std::cerr << filename << ':' << lineNumber;
                std::cerr << "  unknown item type " << parts[1] << '\n';
                ++errorCount;
                continue;
            }


        // TILE SPECIFIC
        } else if (parts[0] == "@tile") {
            if (currentType != TYPE_NONE) {
                std::cerr << filename << ':' << lineNumber;
                std::cerr << "  tried to start tile def while already in def\n";
                ++errorCount;
                continue;
            }
            currentType = TYPE_TILE;
            unsigned ident = strToInt(parts[1]);
            tile.ident = ident;
            tile.name = "unknown";
            tile.desc = "";
            tile.glyph = '?';
            tile.r = 255; tile.g = 0; tile.b = 255;
            tile.isPassable = false;
            tile.isOpaque = false;
            tile.isDownStair = false;
            tile.isUpStair = false;
        } else if (parts[0] == "isPassable") {
            tile.isPassable = true;
        } else if (parts[0] == "isOpaque") {
            tile.isOpaque = true;
        } else if (parts[0] == "isDownStair") {
            tile.isDownStair = true;
        } else if (parts[0] == "isUpStair") {
            tile.isUpStair = true;

        // ALL TYPES SHARED
        } else if (parts[0] == "glyph") {
            if (parts[1].size() != 1) {
                std::cerr << filename << ':' << lineNumber;
                std::cerr << "  glyph must be exactly one character\n";
                ++errorCount;
                continue;
            }
            switch(currentType) {
                case TYPE_ACTOR:
                    actor.glyph = parts[1][0];
                    break;
                case TYPE_ITEM:
                    item.glyph = parts[1][0];
                    break;
                case TYPE_TILE:
                    tile.glyph = parts[1][0];
                    break;
            }
        } else if (parts[0] == "name") {
            std::string nameText = convertUnderscores(parts[1]);
            switch(currentType) {
                case TYPE_ACTOR:
                    actor.name = nameText;
                    break;
                case TYPE_ITEM:
                    item.name = nameText;
                    break;
                case TYPE_TILE:
                    tile.name = nameText;
                    break;
            }
        } else if (parts[0] == "description") {
            std::string descriptionText = convertUnderscores(parts[1]);
            switch(currentType) {
                case TYPE_ACTOR:
                    actor.desc = descriptionText;
                    break;
                case TYPE_ITEM:
                    item.desc = descriptionText;
                    break;
                case TYPE_TILE:
                    tile.desc = descriptionText;
                    break;
            }
        } else if (parts[0] == "colour") {
            int r = strToInt(parts[1]);
            int g = strToInt(parts[2]);
            int b = strToInt(parts[3]);
            switch(currentType) {
                case TYPE_ACTOR:
                    actor.r = r;
                    actor.g = g;
                    actor.b = b;
                    break;
                case TYPE_ITEM:
                    item.r = r;
                    item.g = g;
                    item.b = b;
                    break;
                case TYPE_TILE:
                    tile.r = r;
                    tile.g = g;
                    tile.b = b;
                    break;
            }
        } else if (parts[0] == "effect") {
            EffectData ed;
            ed.trigger = strToInt(parts[1]);
            ed.effectChance = strToInt(parts[1]);
            ed.effectId = strToInt(parts[1]);
            ed.effectStrength = strToInt(parts[1]);
            switch(currentType) {
                case TYPE_TILE:
                case TYPE_ACTOR:
                    std::cerr << filename << ':' << lineNumber;
                    std::cerr << "  effect data not supported.\n";
                    ++errorCount;
                    break;
                case TYPE_ITEM:
                    item.effects.push_back(ed);
                    break;
            }
        } else {
            std::cerr << filename << ':' << lineNumber;
            std::cerr << "  unhandled command or attribute " << parts[0] << ".\n";
            ++errorCount;
        }
    }

    if (errorCount > 0) {
        std::cerr << "FOUND " << errorCount << " ERRORS IN DATA.\n";
        return false;
    } else return true;
}

