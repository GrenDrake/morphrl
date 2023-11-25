#include <iostream>
#include <string>
#include <vector>

#include "morph.h"

std::string triggerEffect(World &world, const EffectData &effect, Actor *user, Actor *target) {
    if (!user) {
        std::cerr << "Tried to trigger effect with no user.\n";
        return "";
    }
    if (!target) target = user;
    if (globalRNG.upto(100) >= effect.effectChance) return "";

    switch (effect.effectId) {
        case EFFECT_HEALING: {
            int amount = effect.effectStrength * user->getStat(STAT_HEALTH) / 100;
            if (amount < 1) amount = 1;
            user->takeDamage(-amount);
            return "Received " + std::to_string(amount) + " healing. "; }
        case EFFECT_DAMAGE: {
            user->takeDamage(effect.effectStrength);
            std::string message = ucFirst(user->getName(true)) + " took "
                    + std::to_string(effect.effectStrength) + " damage. ";
            if (user->isDead()) {
                if (user->isPlayer) {
                    message += "You [color=red]die[/color]. ";
                } else {
                    std::vector<Item*> drops;
                    for (Item *item : user->inventory) drops.push_back(item);
                    user->dropAllItems();
                    message += "They [color=red]die[/color] and drop " + makeItemList(drops, 4) + ". ";
                }
            }
            return message; }
        case EFFECT_APPLY_STATUS: {
            const StatusData &statusData = getStatusData(effect.effectStrength);
            if (statusData.ident == BAD_VALUE) {
                std::cerr << "ERROR: Tried to apply invalid status " << effect.effectStrength << '\n';
                return "";
            } else {
                StatusItem *statusItem = new StatusItem(statusData);
                user->applyStatus(statusItem);
                return "You are now effected by [color=yellow]" + statusData.name + "[/color]. ";
            }
            break; }
        case EFFECT_MUTATE: {
            const MutationData &data = getRandomMutationData();
            if (!user->hasMutation(data.ident)) {
                user->applyMutation(new MutationItem(data));
                return "You mutate, " + data.gainVerb + " " + data.name + "! ";
            } else {
                return "";
            }
            break; }
        default:
            std::cerr << "ERROR: Unhandled effect " << effect.effectId << ".\n";
    }
    return "";
}


std::string EffectData::toString() const {
    if (trigger == ET_BOOST) {
        std::string text;
        if (effectStrength >= 0) text += "[color=cyan]+";
        else text += "[color=red]";
        text += std::to_string(effectStrength) + "[/color] " + statName(effectId);
        return text;
    }

    std::string text;
    switch(trigger) {
        case ET_GIVE_ABILITY: // grant special ability
            text += "gives ability: ";
            break;
        case ET_ON_HIT:
            text += "on hit: ";
            break;
        case ET_ON_USE:
            text += "on use: ";
            break;
        case ET_ON_TICK:
            text += "every turn: ";
            break;
        default:
            text += "Unhandled trigger " + std::to_string(trigger) + ": ";
    }
    if (effectChance < 100) {
        text += "[color=cyan]" + std::to_string(effectChance) + "[/color]% chance to ";
    }
    switch(effectId) {
        case EFFECT_HEALING:
            text += "heal for [color=cyan]" + std::to_string(effectStrength) + "[/color]%";
            break;
        case EFFECT_MUTATE:
            text += "mutate the user";
            break;
        case EFFECT_APPLY_STATUS: {
            const StatusData &statusData = getStatusData(effectStrength);
            text += "apply [color=cyan]" + statusData.name + "[/color]";
            break; }
        default:
            text += "Unhandled effectId " + std::to_string(effectId);
    }
    return text;
}
