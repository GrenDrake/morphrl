#include <algorithm>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include "physfs.h"
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

const DataDef BAD_DEF{ "INVALID", BAD_VALUE };

ActorData BAD_ACTOR{BAD_VALUE, '?', 255, 0, 255, "invalid"};
std::vector<ActorData> actorData;

ItemData BAD_ITEM{BAD_VALUE, '?', 255, 0, 255, "invalid", "", ItemData::Invalid};
std::vector<ItemData> itemData;

StatusData BAD_STATUS{BAD_VALUE, "invalid"};
std::vector<StatusData> statusData;

MutationData BAD_MUTATION{BAD_VALUE, "invalid"};
std::vector<MutationData> mutationData;

AbilityData BAD_ABILITY{BAD_VALUE, "invalid"};
std::vector<AbilityData> abilityData;

TileData BAD_TILE{BAD_VALUE, '?', "invalid"};
std::vector<TileData> tileData;

DungeonData BAD_DUNGEON{BAD_VALUE, "invalid"};
std::vector<DungeonData> dungeonData;


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

const MutationData& getMutationData(unsigned ident) {
    for (const MutationData &data : mutationData) {
        if (data.ident == ident) return data;
    }
    return BAD_MUTATION;
}

const AbilityData& getAbilityData(unsigned ident) {
    for (const AbilityData &data : abilityData) {
        if (data.ident == ident) return data;
    }
    return BAD_ABILITY;
}

const MutationData& getRandomMutationData() {
    if (mutationData.empty()) return BAD_MUTATION;
    unsigned i = globalRNG.upto(mutationData.size());
    return mutationData[i];
}

const StatusData& getStatusData(unsigned ident) {
    for (const StatusData &data : statusData) {
        if (data.ident == ident) return data;
    }
    return BAD_STATUS;
}

const TileData& getTileData(unsigned ident) {
    for (const TileData &data : tileData) {
        if (data.ident == ident) return data;
    }
    return BAD_TILE;
}

const DungeonData& getDungeonData(unsigned ident) {
    for (const DungeonData &data : dungeonData) {
        if (data.ident == ident) return data;
    }
    return BAD_DUNGEON;
}
unsigned getDungeonEntranceIdent() {
    for (const DungeonData &data : dungeonData) {
        if (data.hasEntrance) return data.ident;
    }
    return BAD_VALUE;
}


std::string readFile(const std::string &filename) {
    PHYSFS_File *fp = PHYSFS_openRead(filename.c_str());
    if (!fp) {
        std::string errorMessage = "Failed to read file " + filename + ": ";
        errorMessage += PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode());
        logMessage(LOG_ERROR, errorMessage);
        return "";
    }
    auto length = PHYSFS_fileLength(fp);
    if (length == -1) {
        logMessage(LOG_ERROR, "File " + filename + " is of indeterminate length");
        PHYSFS_close(fp);
        return "";
    }
    char *buffer = new char[length + 1];
    auto bytesRead = PHYSFS_readBytes(fp, buffer, length);
    buffer[length] = 0;
    std::string result(buffer);
    delete[] buffer;
    if (length != bytesRead) {
        std::string errorMessage = "Unexpected file length " + filename + ": ";
        errorMessage += PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode());
        logMessage(LOG_ERROR, errorMessage);
    }
    PHYSFS_close(fp);
    return result;
}

std::vector<unsigned char> readFileAsBinary(const std::string &filename) {
    std::vector<unsigned char> result;
    PHYSFS_File *fp = PHYSFS_openRead(filename.c_str());
    if (!fp) {
        std::string errorMessage = "Failed to read file " + filename + ": ";
        errorMessage += PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode());
        logMessage(LOG_ERROR, errorMessage);
        return result;
    }
    auto length = PHYSFS_fileLength(fp);
    if (length == -1) {
        logMessage(LOG_ERROR, "File " + filename + " is of indeterminate length.");
        PHYSFS_close(fp);
        return result;
    }
    result.reserve(length);
    while (1) {
        char theByte;
        auto readCount = PHYSFS_readBytes(fp, &theByte, 1);
        if (PHYSFS_eof(fp)) break;
        if (readCount != 1) {
            std::string errorMessage = "Read error in " + filename + ": ";
            errorMessage += PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode());
            logMessage(LOG_ERROR, errorMessage);
        }
        result.push_back(theByte);
    }
    PHYSFS_close(fp);
    return result;
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
    std::string fileContent = readFile(filename);
    if (fileContent.empty()) return false;
    std::stringstream inf(fileContent);

    DataTemp *data = nullptr;

    int lineNumber = 0;
    std::string line;
    while (std::getline(inf, line)) {
        ++lineNumber;
        Origin origin(filename, lineNumber);
        auto parts = explodeOnWhitespace(line);
        if (parts.empty() || parts[0][0] == '#') continue;
        if (parts[0] == "@include") {
            if (parts.size() != 2) {
                rawData.addError(origin, "@include requries one argument");
            } else {
                loadRawFromFile(parts[1], rawData);
            }
        } else if (parts[0][0] == '@') {
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
    { "baseLevel",      1 },
    { "artfile",        1 },
    { "description",    1 },
    { "colour",         3 },
    { "item",           3 },
    { "mutation",       3 },
    { "base_strength",  1 },
    { "base_agility",   1 },
    { "base_speed",     1 },
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
    resultData.baseLevel = 1;
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
            } else if (prop.name == "artfile") {
                resultData.artFile = prop.value[0];
            } else if (prop.name == "description") {
                resultData.desc = convertUnderscores(prop.value[0]);
            } else if (prop.name == "item") {
                SpawnLine line;
                line.spawnGroup = dataAsInt(rawData, prop.origin, prop.value[0]);
                line.spawnChance = dataAsInt(rawData, prop.origin, prop.value[1]);
                line.ident = dataAsInt(rawData, prop.origin, prop.value[2]);
                resultData.initialItems.push_back(line);
            } else if (prop.name == "mutation") {
                SpawnLine line;
                line.spawnGroup = dataAsInt(rawData, prop.origin, prop.value[0]);
                line.spawnChance = dataAsInt(rawData, prop.origin, prop.value[1]);
                line.ident = dataAsInt(rawData, prop.origin, prop.value[2]);
                resultData.initialMutations.push_back(line);
            } else if (prop.name == "colour") {
                resultData.r = dataAsInt(rawData, prop.origin, prop.value[0]);
                resultData.g = dataAsInt(rawData, prop.origin, prop.value[1]);
                resultData.b = dataAsInt(rawData, prop.origin, prop.value[2]);
            } else if (prop.name == "baseLevel") {
                resultData.baseLevel = dataAsInt(rawData, prop.origin, prop.value[0]);
            } else if (prop.name == "base_strength") {
                resultData.baseStats[STAT_STRENGTH] = dataAsInt(rawData, prop.origin, prop.value[0]);
            } else if (prop.name == "base_agility") {
                resultData.baseStats[STAT_AGILITY] = dataAsInt(rawData, prop.origin, prop.value[0]);
            } else if (prop.name == "base_speed") {
                resultData.baseStats[STAT_SPEED] = dataAsInt(rawData, prop.origin, prop.value[0]);
            } else if (prop.name == "base_toughness") {
                resultData.baseStats[STAT_TOUGHNESS] = dataAsInt(rawData, prop.origin, prop.value[0]);
            } else {
                rawData.addError(prop.origin, "unhandled property name " + prop.name);
            }
        }
    }

    const ActorData &oldActorData = getActorData(resultData.ident);
    if (oldActorData.ident == resultData.ident) {
        rawData.addError(rawActor->origin, "actor ident " + std::to_string(resultData.ident) + " already used");
        return false;
    } else {
        actorData.push_back(resultData);
        return true;
    }
}

std::vector<DataDef> statusPropData{
    { "name",           1 },
    { "description",    1 },
    { "minDuration",    1 },
    { "maxDuration",    1 },
    { "resistDC",       1 },
    { "resistEveryTurn",0 },
    { "effect",         5 },
};
bool processStatusData(RawData &rawData, const DataTemp *rawStatus) {
    if (!rawStatus || rawStatus->typeName != "@status") {
        rawData.addError(Origin(), "processStatusData passed malformed data");
        return false;
    }

    StatusData resultData;
    resultData.name = "unknown";
    resultData.minDuration = 0;
    resultData.maxDuration = 4294967295;
    resultData.resistDC = 9999;
    resultData.resistEveryTurn = false;

    resultData.ident = rawStatus->ident;
    for (const DataProp &prop : rawStatus->props) {
        const DataDef &dataDef = getDataDef(statusPropData, prop.name);
        if (dataDef.partCount == BAD_VALUE) {
            rawData.addError(prop.origin, "unknown item property " + prop.name);
        } else if (dataDef.partCount != prop.value.size()) {
            rawData.addError(prop.origin, "expected " + std::to_string(dataDef.partCount)
                                          + " values, but found "
                                          + std::to_string(prop.value.size()));
        } else {
            if (prop.name == "name") {
                resultData.name = convertUnderscores(prop.value[0]);
            } else if (prop.name == "description") {
                resultData.desc = convertUnderscores(prop.value[0]);
            } else if (prop.name == "minDuration") {
                resultData.minDuration = dataAsInt(rawData, prop.origin, prop.value[0]);
            } else if (prop.name == "maxDuration") {
                resultData.maxDuration = dataAsInt(rawData, prop.origin, prop.value[0]);
            } else if (prop.name == "resistDC") {
                resultData.resistDC = dataAsInt(rawData, prop.origin, prop.value[0]);
            } else if (prop.name == "resistEveryTurn") {
                resultData.resistEveryTurn = true;
            } else if (prop.name == "effect") {
                EffectData effectData;
                effectData.trigger = dataAsInt(rawData, prop.origin, prop.value[0]);
                effectData.effectChance = dataAsInt(rawData, prop.origin, prop.value[1]);
                effectData.effectId = dataAsInt(rawData, prop.origin, prop.value[2]);
                effectData.effectStrength = dataAsInt(rawData, prop.origin, prop.value[3]);
                effectData.effectParam = dataAsInt(rawData, prop.origin, prop.value[4]);
                resultData.effects.push_back(effectData);
            } else {
                rawData.addError(prop.origin, "unhandled property name " + prop.name);
            }
        }
    }
    if (resultData.minDuration > resultData.maxDuration) {
        rawData.addError(rawStatus->origin, "maximum duration of status is less than its minimum");
        return false;
    }
    const StatusData &oldStatusData = getStatusData(resultData.ident);
    if (oldStatusData.ident == resultData.ident) {
        rawData.addError(rawStatus->origin, "status ident " + std::to_string(resultData.ident) + " already used");
        return false;
    } else {
        statusData.push_back(resultData);
        return true;
    }
}

std::vector<DataDef> mutationPropData{
    { "gainVerb",       1 },
    { "name",           1 },
    { "description",    1 },
    { "slot",           1 },
    { "effect",         5 },
};
bool processMutationData(RawData &rawData, const DataTemp *rawMutation) {
    if (!rawMutation || rawMutation->typeName != "@mutation") {
        rawData.addError(Origin(), "processMutationData passed malformed data");
        return false;
    }

    MutationData resultData;
    resultData.name = "unknown";
    resultData.gainVerb = "gaining";
    resultData.slot = 0;

    resultData.ident = rawMutation->ident;
    for (const DataProp &prop : rawMutation->props) {
        const DataDef &dataDef = getDataDef(mutationPropData, prop.name);
        if (dataDef.partCount == BAD_VALUE) {
            rawData.addError(prop.origin, "unknown mutation property " + prop.name);
        } else if (dataDef.partCount != prop.value.size()) {
            rawData.addError(prop.origin, "expected " + std::to_string(dataDef.partCount)
                                          + " values, but found "
                                          + std::to_string(prop.value.size()));
        } else {
            if (prop.name == "name") {
                resultData.name = convertUnderscores(prop.value[0]);
            } else if (prop.name == "gainVerb") {
                resultData.gainVerb = convertUnderscores(prop.value[0]);
            } else if (prop.name == "description") {
                resultData.desc = convertUnderscores(prop.value[0]);
            } else if (prop.name == "slot") {
                resultData.slot = dataAsInt(rawData, prop.origin, prop.value[0]);
            } else if (prop.name == "effect") {
                EffectData effectData;
                effectData.trigger = dataAsInt(rawData, prop.origin, prop.value[0]);
                effectData.effectChance = dataAsInt(rawData, prop.origin, prop.value[1]);
                effectData.effectId = dataAsInt(rawData, prop.origin, prop.value[2]);
                effectData.effectStrength = dataAsInt(rawData, prop.origin, prop.value[3]);
                effectData.effectParam = dataAsInt(rawData, prop.origin, prop.value[4]);
                resultData.effects.push_back(effectData);
            } else {
                rawData.addError(prop.origin, "unhandled property name " + prop.name);
            }
        }
    }
    const MutationData &oldMutationData = getMutationData(resultData.ident);
    if (oldMutationData.ident == resultData.ident) {
        rawData.addError(rawMutation->origin, "mutation ident " + std::to_string(resultData.ident) + " already used");
        return false;
    } else {
        mutationData.push_back(resultData);
        return true;
    }
}

std::vector<DataDef> abilityPropData{
    { "name",           1 },
    { "description",    1 },
    { "energyCost",     1 },
    { "areaType",       1 },
    { "maxRange",       1 },
    { "speedMult",      1 },
    { "effect",         5 },
};
bool processAbilityData(RawData &rawData, const DataTemp *rawAbility) {
    if (!rawAbility || rawAbility->typeName != "@ability") {
        rawData.addError(Origin(), "processAbilityData passed malformed data");
        return false;
    }

    AbilityData resultData;
    resultData.name = "unknown";
    resultData.energyCost = 0;
    resultData.maxRange = 10;
    resultData.areaType = AR_NONE;
    resultData.speedMult = 100;

    resultData.ident = rawAbility->ident;
    for (const DataProp &prop : rawAbility->props) {
        const DataDef &dataDef = getDataDef(abilityPropData, prop.name);
        if (dataDef.partCount == BAD_VALUE) {
            rawData.addError(prop.origin, "unknown ability property " + prop.name);
        } else if (dataDef.partCount != prop.value.size()) {
            rawData.addError(prop.origin, "expected " + std::to_string(dataDef.partCount)
                                          + " values, but found "
                                          + std::to_string(prop.value.size()));
        } else {
            if (prop.name == "name") {
                resultData.name = convertUnderscores(prop.value[0]);
            } else if (prop.name == "description") {
                resultData.desc = convertUnderscores(prop.value[0]);
            } else if (prop.name == "energyCost") {
                resultData.energyCost = dataAsInt(rawData, prop.origin, prop.value[0]);
            } else if (prop.name == "areaType") {
                resultData.areaType = dataAsInt(rawData, prop.origin, prop.value[0]);
            } else if (prop.name == "maxRange") {
                resultData.maxRange = dataAsInt(rawData, prop.origin, prop.value[0]);
            } else if (prop.name == "speedMult") {
                resultData.speedMult = dataAsInt(rawData, prop.origin, prop.value[0]);
            } else if (prop.name == "effect") {
                EffectData effectData;
                effectData.trigger = dataAsInt(rawData, prop.origin, prop.value[0]);
                effectData.effectChance = dataAsInt(rawData, prop.origin, prop.value[1]);
                effectData.effectId = dataAsInt(rawData, prop.origin, prop.value[2]);
                effectData.effectStrength = dataAsInt(rawData, prop.origin, prop.value[3]);
                effectData.effectParam = dataAsInt(rawData, prop.origin, prop.value[4]);
                resultData.effects.push_back(effectData);
            } else {
                rawData.addError(prop.origin, "unhandled property name " + prop.name);
            }
        }
    }
    const AbilityData &oldAbilityData = getAbilityData(resultData.ident);
    if (oldAbilityData.ident == resultData.ident) {
        rawData.addError(rawAbility->origin, "ability ident " + std::to_string(resultData.ident) + " already used");
        return false;
    } else {
        abilityData.push_back(resultData);
        return true;
    }
}

std::vector<DataDef> itemPropData{
    { "glyph",          1 },
    { "name",           1 },
    { "description",    1 },
    { "colour",         3 },
    { "type",           1 },
    { "bulk",           1 },
    { "damage",         2 },
    { "effect",         5 },
    { "consumeChance",  1 },
    { "isVictoryArtifact", 0 },
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
    resultData.minDamage = 0;
    resultData.maxDamage = 0;
    resultData.consumeChance = 0;
    resultData.isVictoryArtifact = false;

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
            } else if (prop.name == "damage") {
                resultData.minDamage = dataAsInt(rawData, prop.origin, prop.value[0]);
                resultData.maxDamage = dataAsInt(rawData, prop.origin, prop.value[1]);
            } else if (prop.name == "consumeChance") {
                resultData.consumeChance = dataAsInt(rawData, prop.origin, prop.value[0]);
            } else if (prop.name == "isVictoryArtifact") {
                resultData.isVictoryArtifact = true;
            } else if (prop.name == "effect") {
                EffectData effectData;
                effectData.trigger = dataAsInt(rawData, prop.origin, prop.value[0]);
                effectData.effectChance = dataAsInt(rawData, prop.origin, prop.value[1]);
                effectData.effectId = dataAsInt(rawData, prop.origin, prop.value[2]);
                effectData.effectStrength = dataAsInt(rawData, prop.origin, prop.value[3]);
                effectData.effectParam = dataAsInt(rawData, prop.origin, prop.value[4]);
                resultData.effects.push_back(effectData);
            } else {
                rawData.addError(prop.origin, "unhandled property name " + prop.name);
            }
        }
    }
    const ItemData &oldItemData = getItemData(resultData.ident);
    if (oldItemData.ident == resultData.ident) {
        rawData.addError(rawItem->origin, "item ident " + std::to_string(resultData.ident) + " already used");
        return false;
    } else {
        itemData.push_back(resultData);
        return true;
    }
}

std::vector<DataDef> tilePropData{
    { "glyph",          1 },
    { "name",           1 },
    { "colour",         3 },
    { "isOpaque",       0 },
    { "isPassable",     0 },
    { "isUpStair",      0 },
    { "isDownStair",    0 },
    { "preventActorSpawns", 0 },
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
    resultData.preventActorSpawns = false;
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
            } else if (prop.name == "preventActorSpawns") {
                resultData.preventActorSpawns = true;
            } else {
                rawData.addError(prop.origin, "unhandled property name " + prop.name);
            }
        }
    }

    const TileData &oldTileData = getTileData(resultData.ident);
    if (oldTileData.ident == resultData.ident) {
        rawData.addError(rawTile->origin, "tile ident " + std::to_string(resultData.ident) + " already used");
        return false;
    } else {
        tileData.push_back(resultData);
        return true;
    }
}

std::vector<DataDef> dungeonPropData{
    { "name",           1 },
    { "hasEntrance",    0 },
    { "noUpStairs",     0 },
    { "noDownStairs",   0 },
    { "actorCount",     1 },
    { "actor",          3 },
    { "itemCount",      1 },
    { "item",           3 },
};
bool processDungeonData(RawData &rawData, const DataTemp *rawDungeon) {
    if (!rawDungeon || rawDungeon->typeName != "@dungeon") {
        rawData.addError(Origin(), "processDungeonData passed malformed data");
        return false;
    }

    DungeonData resultData;
    resultData.name = "unknown";
    resultData.hasEntrance = false;
    resultData.hasUpStairs = true;
    resultData.hasDownStairs = true;
    resultData.ident = rawDungeon->ident;
    resultData.actorCount = 0;
    resultData.itemCount = 0;
    for (const DataProp &prop : rawDungeon->props) {
        const DataDef &dataDef = getDataDef(dungeonPropData, prop.name);
        if (dataDef.partCount == BAD_VALUE) {
            rawData.addError(prop.origin, "unknown dungeon property " + prop.name);
        } else if (dataDef.partCount != prop.value.size()) {
            rawData.addError(prop.origin, "expected " + std::to_string(dataDef.partCount)
                                          + " values, but found "
                                          + std::to_string(prop.value.size()));
        } else {
            if (prop.name == "name") {
                resultData.name = convertUnderscores(prop.value[0]);
            } else if (prop.name == "hasEntrance") {
                resultData.hasEntrance = true;
            } else if (prop.name == "noUpStairs") {
                resultData.hasUpStairs = false;
            } else if (prop.name == "noDownStairs") {
                resultData.hasDownStairs = false;
            } else if (prop.name == "actorCount") {
                resultData.actorCount = dataAsInt(rawData, prop.origin, prop.value[0]);
            } else if (prop.name == "actor") {
                SpawnLine spawnLine;
                spawnLine.spawnGroup  = dataAsInt(rawData, prop.origin, prop.value[0]);
                spawnLine.spawnChance = dataAsInt(rawData, prop.origin, prop.value[1]);
                spawnLine.ident       = dataAsInt(rawData, prop.origin, prop.value[2]);
                resultData.actorSpawns.push_back(spawnLine);
            } else if (prop.name == "itemCount") {
                resultData.itemCount = dataAsInt(rawData, prop.origin, prop.value[0]);
            } else if (prop.name == "item") {
                SpawnLine spawnLine;
                spawnLine.spawnGroup  = dataAsInt(rawData, prop.origin, prop.value[0]);
                spawnLine.spawnChance = dataAsInt(rawData, prop.origin, prop.value[1]);
                spawnLine.ident       = dataAsInt(rawData, prop.origin, prop.value[2]);
                resultData.itemSpawns.push_back(spawnLine);
            } else {
                rawData.addError(prop.origin, "unhandled property name " + prop.name);
            }
        }
    }

    const DungeonData &oldDungeonData = getDungeonData(resultData.ident);
    if (oldDungeonData.ident == resultData.ident) {
        rawData.addError(rawDungeon->origin, "dungeon ident " + std::to_string(resultData.ident) + " already used");
        return false;
    } else {
        dungeonData.push_back(resultData);
        return true;
    }
}

template<class T>
void sortDataEntries(std::vector<T> &data) {
    std::sort(data.begin(), data.end(), [](const T &lhs, const T &rhs){ return lhs.ident < rhs.ident; });
}

bool loadAllData() {
    RawData rawData;
    loadRawFromFile("game.dat", rawData);

    if (rawData.hasErrors()) {
        for (const ErrorMessage &msg : rawData.errors) {
            logMessage(LOG_ERROR, msg.origin.toString() + "  " + msg.message);
        }
        return false;
    }

    int maxItem = 0, maxTile = 0, maxActor = 0, maxStatus = 0, maxDungeon = 0;
    int maxAbility = 0, maxMutation = 0;
    for (const DataTemp *data : rawData.data) {
        if (!data) continue;
        if (data->typeName == "@define") {
            // we don't need to do anything for this case
        } else if (data->typeName == "@tile") {
            if (maxTile < data->ident) maxTile = data->ident;
            processTileData(rawData, data);
        } else if (data->typeName == "@actor") {
            if (maxActor < data->ident) maxActor = data->ident;
            processActorData(rawData, data);
        } else if (data->typeName == "@item") {
            if (maxItem < data->ident) maxItem = data->ident;
            processItemData(rawData, data);
        } else if (data->typeName == "@ability") {
            if (maxAbility < data->ident) maxAbility = data->ident;
            processAbilityData(rawData, data);
        } else if (data->typeName == "@status") {
            if (maxStatus < data->ident) maxStatus = data->ident;
            processStatusData(rawData, data);
        } else if (data->typeName == "@mutation") {
            if (maxMutation < data->ident) maxMutation = data->ident;
            processMutationData(rawData, data);
        } else if (data->typeName == "@dungeon") {
            if (maxDungeon < data->ident) maxDungeon = data->ident;
            processDungeonData(rawData, data);
        } else {
            rawData.addError(data->origin, "unknown object type " + data->typeName);
        }
    }

    sortDataEntries(tileData);
    sortDataEntries(itemData);
    sortDataEntries(actorData);
    sortDataEntries(statusData);
    sortDataEntries(mutationData);
    sortDataEntries(dungeonData);

    logMessage(LOG_INFO, "LOADED " + std::to_string(tileData.size())     + " tiles (next ident: "          + std::to_string(maxTile+1)     + ")");
    logMessage(LOG_INFO, "LOADED " + std::to_string(itemData.size())     + " items (next ident: "          + std::to_string(maxItem+1)     + ")");
    logMessage(LOG_INFO, "LOADED " + std::to_string(actorData.size())    + " actors (next ident: "         + std::to_string(maxActor+1)    + ")");
    logMessage(LOG_INFO, "LOADED " + std::to_string(statusData.size())   + " status effects (next ident: " + std::to_string(maxStatus+1)   + ")");
    logMessage(LOG_INFO, "LOADED " + std::to_string(abilityData.size())  + " abilities (next ident: "      + std::to_string(maxAbility+1)  + ")");
    logMessage(LOG_INFO, "LOADED " + std::to_string(mutationData.size()) + " mutations (next ident: "      + std::to_string(maxMutation+1) + ")");
    logMessage(LOG_INFO, "LOADED " + std::to_string(dungeonData.size())  + " dungeon levels (next ident: " + std::to_string(maxDungeon+1)  + ")");

    if (rawData.hasErrors()) {
        for (const ErrorMessage &msg : rawData.errors) {
            logMessage(LOG_ERROR, msg.origin.toString() + "  " + msg.message);
        }
        return false;
    } else {
        return true;
    }
}

