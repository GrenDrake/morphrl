#include <iostream>
#include <string>
#include <vector>

#include "morph.h"

std::string triggerEffect(const EffectData &effect, Actor *user, Actor *target) {
    if (!user) {
        std::cerr << "Tried to trigger effect with no user.\n";
        return "";
    }
    if (!target) target = user;
    if (globalRNG.upto(100) >= effect.effectChance) return "";

    switch (effect.effectId) {
        case EFFECT_HEALING: {
            int max = effect.effectParam;
            int min = effect.effectStrength;
            int range = max - min;
            int amount = globalRNG.upto(range) + min;
            if (amount < 1) amount = 1;
            target->takeDamage(-amount);
            return "[color=yellow]" + ucFirst(target->getName(true)) + "[/color] received [color=green]" + std::to_string(amount) + "[/color] healing. ";
            }
        case EFFECT_ADJ_ENERGY: {
            int max = effect.effectParam;
            int min = effect.effectStrength;
            int range = max - min;
            int amount = globalRNG.upto(range) + min;
            if (amount < 1) amount = 1;
            target->spendEnergy(-amount);
            return "[color=yellow]" + ucFirst(target->getName(true)) + "[/color] regained [color=green]" + std::to_string(amount) + "[/color] energy. ";
            }
        case EFFECT_DAMAGE: {
            int max = effect.effectParam;
            int min = effect.effectStrength;
            int range = max - min;
            int amount = globalRNG.upto(range) + min;
            target->takeDamage(amount);
            std::string message = "[color=yellow]" + ucFirst(target->getName(true)) + "[/color] took [color=red]"
                    + std::to_string(amount) + "[/color] damage. ";
            if (target->isDead()) {
                if (target->isPlayer) {
                    message += "You [color=red]die[/color]. ";
                } else {
                    std::vector<Item*> drops;
                    for (Item *item : target->inventory) drops.push_back(item);
                    target->dropAllItems();
                    message += "They [color=red]die[/color] and drop " + makeItemList(drops, 4) + ". ";
                }
            }
            return message; }
        case EFFECT_APPLY_STATUS: {
            if (target->hasStatus(effect.effectStrength)) return ""; // prevent stacking status effects
            const StatusData &statusData = getStatusData(effect.effectStrength);
            if (statusData.ident == BAD_VALUE) {
                std::cerr << "ERROR: Tried to apply invalid status " << effect.effectStrength << '\n';
                return "";
            } else {
                std::string message;
                if (statusData.resistDC < 1000) {
                    int roll = globalRNG.upto(20);
                    int stat = target->getStat(STAT_TOUGHNESS);
                    message = "[[" + std::to_string(roll) + "+" + std::to_string(stat);
                    message += " vs " + std::to_string(statusData.resistDC) + "]] ";
                    if (roll + stat >= statusData.resistDC) { // effect was resisted
                        message += "[color=yellow]" + ucFirst(target->getName(true)) + "[/color] resisted the [color=yellow]";
                        message += statusData.name + "[/color] effect. ";
                        return message;
                    }
                }
                StatusItem *statusItem = new StatusItem(statusData);
                target->applyStatus(statusItem);
                if (target->isPlayer) message = "[color=yellow]You[/color] are";
                else message = "[color=yellow]" + ucFirst(target->getName(true)) + "[/color] is";
                message += " now effected by [color=yellow]" + statusData.name + "[/color]. ";
                return message;
            }
            break; }
        case EFFECT_PURIFY: {
            if (target->mutations.empty()) return ""; // no mutations to remove
            unsigned index = globalRNG.upto(target->mutations.size());
            MutationItem *which = target->mutations[index];
            target->removeMutation(which);
            std::string message = "[color=yellow]You[/color] no longer have [color=yellow]" + which->data.name + "[/color]. ";
            delete which;
            return message; }
        case EFFECT_MUTATE: {
            const MutationData &data = getRandomMutationData();
            if (!target->hasMutation(data.ident)) {
                target->applyMutation(new MutationItem(data));
                return "[color=yellow]You[/color] mutate, " + data.gainVerb + " [color=yellow]" + data.name + "[/color]! ";
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
    } else if (trigger == ET_UNARMED_ATTACK) {
        const ItemData &itemData = getItemData(effectId);
        std::string text = "unarmed attack [color=yellow]";
        if (itemData.ident == BAD_VALUE) text += "invalid item #" + std::to_string(effectId);
        else text += itemData.name;
        text += "[/color]";
        return text;
    } else if (trigger == ET_GIVE_ABILITY) {
        const AbilityData &abilityData = getAbilityData(effectId);
        std::string text = "gives ability: [color=yellow]";
        if (abilityData.ident == BAD_VALUE) text += "invalid ability #" + std::to_string(effectId);
        else text += abilityData.name;
        text += "[/color]";
        return text;
    }

    std::string text;
    switch(trigger) {
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
        case EFFECT_DAMAGE:
            text += "cause [color=red]" + std::to_string(effectStrength) + "[/color] to [color=red]" + std::to_string(effectParam) + "[/color] damage";
            break;
        case EFFECT_ADJ_ENERGY:
            text += "recover [color=cyan]" + std::to_string(effectStrength) + "[/color] to [color=cyan]" + std::to_string(effectParam) + "[/color] energy";
            break;
        case EFFECT_HEALING:
            text += "heal for [color=cyan]" + std::to_string(effectStrength) + "[/color] to [color=cyan]" + std::to_string(effectParam) + "[/color]";
            break;
        case EFFECT_MUTATE:
            text += "mutate the user";
            break;
        case EFFECT_PURIFY:
            text += "removes a mutation";
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
