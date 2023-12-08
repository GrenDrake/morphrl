#include <algorithm>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include "physfs.h"
#include "morph.h"


struct ErrorMessage {
    Origin origin;
    std::string message;
};

const ConfigValue BAD_CONFIG_VALUE{ "" };
const ConfigValue& ConfigData::getRawValue(const std::string &name) const {
    for (const ConfigValue &v : values) {
        if (v.name == name) return v;
    }
    return BAD_CONFIG_VALUE;
}

bool ConfigData::hasValue(const std::string &name) const {
    return !getRawValue(name).name.empty();
}

bool ConfigData::isInt(const std::string &name) const {
    const ConfigValue &v = getRawValue(name);
    return !v.name.empty() && v.isInt;
}

const std::string& ConfigData::getValue(const std::string &name, const std::string &defaultValue) const {
    if (hasValue(name)) return getRawValue(name).value;
    return defaultValue;
}

int ConfigData::getIntValue(const std::string &name, int defaultValue) const {
    if (isInt(name)) return getRawValue(name).asInt;
    return defaultValue;
}

bool ConfigData::getBoolValue(const std::string &name, bool defaultValue) {
    const ConfigValue &v = getRawValue(name);
    if (v.name.empty()) return defaultValue;
    if (v.isInt) return v.asInt;
    if (v.value == "true" || v.value == "TRUE") return true;
    return false;
}


bool loadConfigData(const std::string &filename, ConfigData &configData) {
    std::string fileContent = readFile("/root/" + filename);
    if (fileContent.empty()) return false;
    std::stringstream inf(fileContent);

    std::vector<ErrorMessage> errors;

    int lineNumber = 0;
    std::string line;
    while (std::getline(inf, line)) {
        ++lineNumber;
        Origin origin(filename, lineNumber);
        auto parts = explodeOnWhitespace(line);
        if (parts.empty() || parts[0][0] == '#') continue;
        if (parts.size() != 2) {
            errors.push_back(ErrorMessage{origin, "unexpected number of values"});
            continue;
        }

        ConfigValue value;
        value.name = parts[0];
        value.value = parts[1];
        value.isInt = strToInt(value.value, value.asInt);
        configData.values.push_back(value);
    }

    if (errors.empty()) {
        return true;
    }
    for (const ErrorMessage &msg : errors) {
        logMessage(LOG_ERROR, msg.origin.toString() + "  " + msg.message);
    }
    return false;
}
