
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
    if (statNumber == STAT_BULK) return 0;

    int result = 0;
    for (const EffectData &effect : data.effects) {
        if (effect.trigger == ET_BOOST && effect.effectId == statNumber) {
            result += effect.effectStrength;
        }
    }

    return result;
}

void activateItem(World &world, Item *item, Actor *user) {
    std::string msg;
    bool didEffect = false;
    if (user->isDead()) return;
    if (user == world.player)   msg = "Used ";
    else                        msg = ucFirst(user->getName()) + " used ";
    msg += "[color=yellow]" + item->getName() + "[/color]. ";

    for (const EffectData &data : item->data.effects) {
        if (data.trigger != ET_ON_USE) continue;
        std::string result = triggerEffect(data, user, nullptr);
        if (!result.empty()) {
            msg += result;
            didEffect = true;
        }
    }

    if (!didEffect) {
        msg += "Nothing happens. ";
    }
    int roll = globalRNG.upto(100);
    if (item->data.consumeChance > roll) {
        user->removeItem(item);
        if (item->data.consumeChance < 100) {
            msg += "[color=yellow]" + ucFirst(item->getName(true)) + "[/color] was used up.";
        }
        delete item;
    }
    world.addMessage(msg);

}
