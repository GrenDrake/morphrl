#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "morph.h"


struct Origin {
    Origin();
    Origin(const std::string &filename, unsigned lineNumber);
    std::string toString() const;

    std::string filename;
    unsigned lineNumber;
};

struct ErrorMessage {
    Origin origin;
    std::string message;
};

struct DataProp {
    Origin origin;
    std::string name;
    std::vector<std::string> value;
};

struct DataTemp {
    Origin origin;
    std::string typeName;
    int ident;
    std::string idName;
    std::vector<DataProp> props;
};

struct DefineValue {
    Origin origin;
    std::string name;
    int value;
};

struct RawData {
    ~RawData();
    void addError(const Origin &origin, const std::string &message);
    bool hasErrors() const { return !errors.empty(); }
    bool addDefine(const Origin &origin, const std::string &name, int value);
    const DefineValue* getDefine(const std::string &name);

    std::vector<DefineValue> defines;
    std::vector<DataTemp*> data;
    std::vector<ErrorMessage> errors;
};

struct DataDef {
    std::string name;
    unsigned partCount;
};

const unsigned BAD_VALUE = 4294967295;
const DataDef BAD_DEF{ "INVALID", BAD_VALUE };
ActorData BAD_ACTOR{BAD_VALUE, '?', 255, 0, 255, "invalid"};
std::vector<ActorData> actorData;
ItemData BAD_ITEM{BAD_VALUE, '?', 255, 0, 255, "invalid", "", ItemData::Invalid};
std::vector<ItemData> itemData;
TileData BAD_TILE{BAD_VALUE, '?', "invalid"};
std::vector<TileData> tileData;


Origin::Origin()
: filename("(internal)", 0)
{ }

Origin::Origin(const std::string &filename, unsigned lineNumber)
: filename(filename), lineNumber(lineNumber)
{ }

std::string Origin::toString() const {
    return filename + ": " + std::to_string(lineNumber);
}

RawData::~RawData() {
    for (DataTemp *iter : data) {
        if (iter) delete iter;
    }
}

void RawData::addError(const Origin &origin, const std::string &message) {
    errors.push_back(ErrorMessage{origin, message});
}

bool RawData::addDefine(const Origin &origin, const std::string &name, int value) {
    const DefineValue *oldValue = getDefine(name);
    if (oldValue) return false;

    defines.push_back(DefineValue{origin, name, value});
    return true;
}

const DefineValue* RawData::getDefine(const std::string &name) {
    for (const DefineValue &def : defines) {
        if (def.name == name) return &def;
    }
    return nullptr;
}


const ActorData& getActorData(unsigned ident) {
    for (const ActorData &data : actorData) {
        if (data.ident == ident) return data;
    }
    return BAD_ACTOR;
}

const ItemData& getItemData(unsigned ident) {
    for (const ItemData &data : itemData) {
        if (data.ident == ident) return data;
    }
    return BAD_ITEM;
}

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

int dataAsInt(RawData &rawData, const Origin &origin, const std::string &text) {
    int value;
    bool result = strToInt(text, value);
    if (result) return value;
    const DefineValue *def = rawData.getDefine(text);
    if (def) return def->value;
    rawData.addError(origin, "expected integer, but found " + text);
    return 0;
}

const DataDef& getDataDef(const std::vector<DataDef> &defs, const std::string &text) {
    for (const DataDef &def : defs) {
        if (def.name == text) return def;
    }
    return BAD_DEF;
}



bool loadRawFromFile(const std::string &filename, RawData &rawData) {
    std::ifstream inf(filename);
    if (!inf) {
        rawData.addError(Origin(filename, 0), "failed to open " + filename + " for reading.");
        return false;
    }

    DataTemp *data = nullptr;

    int lineNumber = 0;
    std::string line;
    while (std::getline(inf, line)) {
        ++lineNumber;
        Origin origin(filename, lineNumber);
        auto parts = explodeOnWhitespace(line);
        if (parts.empty() || parts[0][0] == '#') continue;
        if (parts[0][0] == '@') {
            if (parts.size() != 3) {
                rawData.addError(origin, "malformed object defintition");
                continue;
            }
            data = new DataTemp;
            rawData.data.push_back(data);
            data->origin = origin;
            data->typeName = parts[0];
            data->ident = dataAsInt(rawData, origin, parts[2]);
            if (parts[1] != "-") {
                data->idName = parts[1];
                if (!rawData.addDefine(origin, data->idName, data->ident)) {
                    const DefineValue *oldDefine = rawData.getDefine(data->idName);
                    if (oldDefine) {
                        rawData.addError(origin, data->idName + " already defined at " + oldDefine->origin.toString());
                    } else {
                        rawData.addError(origin, "failed to add define " + data->idName);
                    }
                }
            }
        } else if (data == nullptr) {
            rawData.addError(origin, "tried to add property outside of object def");
        } else {
            DataProp prop;
            prop.origin = origin;
            prop.name = parts[0];
            for (unsigned i = 1; i < parts.size(); ++i) {
                prop.value.push_back(parts[i]);
            }
            data->props.push_back(prop);
        }

    }
    return !rawData.hasErrors();
}


std::vector<DataDef> actorPropData{
    { "glyph",          1 },
    { "name",           1 },
    { "description",    1 },
    { "colour",         3 },
    { "base_strength",  1 },
    { "base_agility",   1 },
    { "base_dexterity", 1 },
    { "base_toughness", 1 },
};
bool processActorData(RawData &rawData, const DataTemp *rawActor) {
    if (!rawActor || rawActor->typeName != "@actor") {
        rawData.addError(Origin(), "processActorData passed malformed data");
        return false;
    }

    ActorData resultData;
    resultData.name = "unknown";
    resultData.glyph = '?';
    resultData.r = 255; resultData.g = 255; resultData.b = 255;
    for (int i = 0; i < STAT_BASE_COUNT; ++i) {
        resultData.baseStats[i] = 1;
    }
    resultData.ident = rawActor->ident;
    for (const DataProp &prop : rawActor->props) {
        const DataDef &dataDef = getDataDef(actorPropData, prop.name);
        if (dataDef.partCount == BAD_VALUE) {
            rawData.addError(prop.origin, "unknown actor property " + prop.name);
        } else if (dataDef.partCount != prop.value.size()) {
            rawData.addError(prop.origin, "expected " + std::to_string(dataDef.partCount)
                                          + " values, but found "
                                          + std::to_string(prop.value.size()));
        } else {
            if (prop.name == "glyph") {
                if (prop.value[0].size() != 1) {
                    rawData.addError(prop.origin, "glyph must be single character");
                } else resultData.glyph = prop.value[0][0];
            } else if (prop.name == "name") {
                resultData.name = convertUnderscores(prop.value[0]);
            } else if (prop.name == "description") {
                resultData.desc = convertUnderscores(prop.value[0]);
            } else if (prop.name == "colour") {
                resultData.r = dataAsInt(rawData, prop.origin, prop.value[0]);
                resultData.g = dataAsInt(rawData, prop.origin, prop.value[1]);
                resultData.b = dataAsInt(rawData, prop.origin, prop.value[2]);
            } else if (prop.name == "base_strength") {
                resultData.baseStats[STAT_STRENGTH] = dataAsInt(rawData, prop.origin, prop.value[0]);
            } else if (prop.name == "base_agility") {
                resultData.baseStats[STAT_AGILITY] = dataAsInt(rawData, prop.origin, prop.value[0]);
            } else if (prop.name == "base_dexterity") {
                resultData.baseStats[STAT_DEXTERITY] = dataAsInt(rawData, prop.origin, prop.value[0]);
            } else if (prop.name == "base_toughness") {
                resultData.baseStats[STAT_TOUGHNESS] = dataAsInt(rawData, prop.origin, prop.value[0]);
            } else {
                rawData.addError(prop.origin, "unhandled property name " + prop.name);
            }
        }
    }
    actorData.push_back(resultData);

    return true;
}

std::vector<DataDef> itemPropData{
    { "glyph",          1 },
    { "name",           1 },
    { "description",    1 },
    { "colour",         3 },
    { "type",           1 },
    { "bulk",           1 },
    { "to_hit",         1 },
    { "damage",         2 },
    { "effect",         4 },
    { "consumeChance",  1 },
};
bool processItemData(RawData &rawData, const DataTemp *rawItem) {
    if (!rawItem || rawItem->typeName != "@item") {
        rawData.addError(Origin(), "processItemData passed malformed data");
        return false;
    }

    ItemData resultData;
    resultData.name = "unknown";
    resultData.glyph = '?';
    resultData.r = 255; resultData.g = 255; resultData.b = 255;
    resultData.type = ItemData::Junk;
    resultData.bulk = 1;
    resultData.toHit = 0;
    resultData.minDamage = 0;
    resultData.maxDamage = 0;
    resultData.consumeChance = 0;

    resultData.ident = rawItem->ident;
    for (const DataProp &prop : rawItem->props) {
        const DataDef &dataDef = getDataDef(itemPropData, prop.name);
        if (dataDef.partCount == BAD_VALUE) {
            rawData.addError(prop.origin, "unknown item property " + prop.name);
        } else if (dataDef.partCount != prop.value.size()) {
            rawData.addError(prop.origin, "expected " + std::to_string(dataDef.partCount)
                                          + " values, but found "
                                          + std::to_string(prop.value.size()));
        } else {
            if (prop.name == "glyph") {
                if (prop.value[0].size() != 1) {
                    rawData.addError(prop.origin, "glyph must be single character");
                } else resultData.glyph = prop.value[0][0];
            } else if (prop.name == "name") {
                resultData.name = convertUnderscores(prop.value[0]);
            } else if (prop.name == "description") {
                resultData.desc = convertUnderscores(prop.value[0]);
            } else if (prop.name == "colour") {
                resultData.r = dataAsInt(rawData, prop.origin, prop.value[0]);
                resultData.g = dataAsInt(rawData, prop.origin, prop.value[1]);
                resultData.b = dataAsInt(rawData, prop.origin, prop.value[2]);
            } else if (prop.name == "type") {
                resultData.type = static_cast<ItemData::Type>(dataAsInt(rawData, prop.origin, prop.value[0]));
            } else if (prop.name == "bulk") {
                resultData.bulk = dataAsInt(rawData, prop.origin, prop.value[0]);
            } else if (prop.name == "to_hit") {
                resultData.toHit = dataAsInt(rawData, prop.origin, prop.value[0]);
            } else if (prop.name == "damage") {
                resultData.minDamage = dataAsInt(rawData, prop.origin, prop.value[0]);
                resultData.maxDamage = dataAsInt(rawData, prop.origin, prop.value[1]);
            } else if (prop.name == "consumeChance") {
                resultData.consumeChance = dataAsInt(rawData, prop.origin, prop.value[0]);
            } else if (prop.name == "effect") {
                EffectData effectData;
                effectData.trigger = dataAsInt(rawData, prop.origin, prop.value[0]);
                effectData.effectChance = dataAsInt(rawData, prop.origin, prop.value[1]);
                effectData.effectId = dataAsInt(rawData, prop.origin, prop.value[2]);
                effectData.effectStrength = dataAsInt(rawData, prop.origin, prop.value[3]);
                resultData.effects.push_back(effectData);
            } else {
                rawData.addError(prop.origin, "unhandled property name " + prop.name);
            }
        }
    }
    itemData.push_back(resultData);

    return true;
}

std::vector<DataDef> tilePropData{
    { "glyph",          1 },
    { "name",           1 },
    { "description",    1 },
    { "colour",         3 },
    { "isOpaque",       0 },
    { "isPassable",     0 },
    { "isUpStair",      0 },
    { "isDownStair",    0 },
};
bool processTileData(RawData &rawData, const DataTemp *rawTile) {
    if (!rawTile || rawTile->typeName != "@tile") {
        rawData.addError(Origin(), "processTileData passed malformed data");
        return false;
    }

    TileData resultData;
    resultData.name = "unknown";
    resultData.glyph = '?';
    resultData.r = 255; resultData.g = 255; resultData.b = 255;
    resultData.isOpaque = false;
    resultData.isPassable = false;
    resultData.isUpStair = false;
    resultData.isDownStair = false;
    resultData.ident = rawTile->ident;
    for (const DataProp &prop : rawTile->props) {
        const DataDef &dataDef = getDataDef(tilePropData, prop.name);
        if (dataDef.partCount == BAD_VALUE) {
            rawData.addError(prop.origin, "unknown tile property " + prop.name);
        } else if (dataDef.partCount != prop.value.size()) {
            rawData.addError(prop.origin, "expected " + std::to_string(dataDef.partCount)
                                          + " values, but found "
                                          + std::to_string(prop.value.size()));
        } else {
            if (prop.name == "glyph") {
                if (prop.value[0].size() != 1) {
                    rawData.addError(prop.origin, "glyph must be single character");
                } else resultData.glyph = prop.value[0][0];
            } else if (prop.name == "name") {
                resultData.name = convertUnderscores(prop.value[0]);
            } else if (prop.name == "description") {
                resultData.desc = convertUnderscores(prop.value[0]);
            } else if (prop.name == "colour") {
                resultData.r = dataAsInt(rawData, prop.origin, prop.value[0]);
                resultData.g = dataAsInt(rawData, prop.origin, prop.value[1]);
                resultData.b = dataAsInt(rawData, prop.origin, prop.value[2]);
            } else if (prop.name == "isOpaque") {
                resultData.isOpaque = true;
            } else if (prop.name == "isPassable") {
                resultData.isPassable = true;
            } else if (prop.name == "isUpStair") {
                resultData.isUpStair = true;
            } else if (prop.name == "isDownStair") {
                resultData.isDownStair = true;
            } else {
                rawData.addError(prop.origin, "unhandled property name " + prop.name);
            }
        }
    }
    tileData.push_back(resultData);

    return true;
}


bool loadAllData() {
    RawData rawData;
    loadRawFromFile("resources/game.dat", rawData);

    if (rawData.hasErrors()) {
        for (const ErrorMessage &msg : rawData.errors) {
            std::cerr << msg.origin.toString() << "  " << msg.message << '\n';
        }
        return false;
    }

    for (const DataTemp *data : rawData.data) {
        if (!data) continue;
        if (data->typeName == "@define") {
            // we don't need to do anything for this case
        } else if (data->typeName == "@tile") {
            processTileData(rawData, data);
        } else if (data->typeName == "@actor") {
            processActorData(rawData, data);
        } else if (data->typeName == "@item") {
            processItemData(rawData, data);
        } else {
            rawData.addError(data->origin, "unknown object type " + data->typeName);
        }
    }

    // for (const DataTemp *data : rawData.data) {
        // std::cerr << "-> '" << data->typeName << "' (" << data->ident << ") '" << data->idName << "'\n";
        // for (const DataProp &prop : data->props) {
            // std::cerr << "    '" << prop.name << "' =";
            // for (const std::string &text : prop.value) {
                // std::cerr << " '" << text << '\'';
            // }
            // std::cerr << '\n';
        // }
    // }

    // for (const DefineValue &def : rawData.defines) {
        // std::cerr << def.origin.toString() << "  " << def.name << " = " << def.value << '\n';
    // }

    if (rawData.hasErrors()) {
        for (const ErrorMessage &msg : rawData.errors) {
            std::cerr << msg.origin.toString() << "  " << msg.message << '\n';
        }
        return false;
    }

    std::cerr << "LOADED " << tileData.size() << " tiles\n";
    std::cerr << "LOADED " << itemData.size() << " items\n";
    std::cerr << "LOADED " << actorData.size() << " actors\n";
    return true;
}

