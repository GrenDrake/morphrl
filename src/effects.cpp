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
            int amount = effect.effectStrength * target->getStat(STAT_HEALTH) / 100;
            if (amount < 1) amount = 1;
            target->takeDamage(-amount);
            return "Received " + std::to_string(amount) + " healing. "; }
        case EFFECT_DAMAGE: {
            target->takeDamage(effect.effectStrength);
            std::string message = ucFirst(target->getName(true)) + " took "
                    + std::to_string(effect.effectStrength) + " damage. ";
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
            const StatusData &statusData = getStatusData(effect.effectStrength);
            if (statusData.ident == BAD_VALUE) {
                std::cerr << "ERROR: Tried to apply invalid status " << effect.effectStrength << '\n';
                return "";
            } else {
                StatusItem *statusItem = new StatusItem(statusData);
                target->applyStatus(statusItem);
                std::string message;
                if (target->isPlayer) message = "You are";
                else message = ucFirst(target->getName(true)) + " is";
                message += " now effected by [color=yellow]" + statusData.name + "[/color]. ";
                return message;
            }
            break; }
        case EFFECT_MUTATE: {
            const MutationData &data = getRandomMutationData();
            if (!target->hasMutation(data.ident)) {
                target->applyMutation(new MutationItem(data));
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
    } else if (trigger == ET_UNARMED_ATTACK) {
        const ItemData &itemData = getItemData(effectId);
        std::string text = "unarmed attack [color=yellow]";
        if (itemData.ident == BAD_VALUE) text += "invalid item #" + std::to_string(effectId);
        else text += itemData.name;
        text += "[/color]";
        return text;
    }

    std::string text;
    switch(trigger) {
        case ET_GIVE_ABILITY:
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
        case EFFECT_DAMAGE:
            text += "cause [color=red]" + std::to_string(effectStrength) + "[/color] damage";
            break;
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
