
#include "morph.h"

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
