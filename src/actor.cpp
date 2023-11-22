#include <iostream>
#include <vector>

#include "morph.h"

// const int EFFECT_HEALING    = 0; // instant healing - strength = percent healed
// const int EFFECT_REGEN      = 1; // regen status - strength = regen rate
// const int EFFECT_MUTATE     = 2; // add mutation - strength = # to add
// const int EFFECT_PURIFY     = 3; // purify mutation - strength = # to remove
// const int EFFECT_POISON     = 4; // poison status - strength = damage rate
// const int EFFECT_BOOST      = 5; // stat boost status - strength = ?


void addUniqueToVector(std::vector<int> &v, int item) {
    for (const int &iter : v) {
        if (iter == item) return;
    }
    v.push_back(item);
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
            text += "when used: ";
            break;
        case ET_ON_TICK:
            text += "every turn: ";
            break;
        default:
            text = "Unhandled trigger " + std::to_string(trigger);
    }
    switch(effectId) {
        case EFFECT_HEALING:
            text += "instantly heal for [color=cyan]" + std::to_string(effectStrength) + "%";
            break;
        default:
            text = "Unhandled effectId " + std::to_string(trigger);
    }
    return text;
}


StatusItem::StatusItem(const StatusData &data)
: data(data), duration(0)
{ }


Actor* Actor::create(const ActorData &data) {
    if (data.ident == BAD_VALUE) return nullptr;
    Actor *actor = new Actor(data, 0);
    if (!actor) return nullptr;

    std::vector<int> spawnGroups;

    for (const SpawnLine &line : data.initialItems) {
        if (line.spawnGroup == 0) {
            int roll = globalRNG.upto(100);
            if (roll < line.spawnChance) {
                Item *item = new Item(getItemData(line.ident));
                if (item) {
                    actor->addItem(item);
                    actor->tryEquipItem(item);
                }
            }
        } else {
            addUniqueToVector(spawnGroups, line.spawnGroup);
        }
    }

    for (int groupId : spawnGroups) {
        int roll = globalRNG.upto(100);
        for (const SpawnLine &line : data.initialItems) {
            if (line.spawnGroup != groupId) continue;
            if (roll < line.spawnChance) {
                const ItemData &itemData = getItemData(line.ident);
                Item *item = new Item(itemData);
                if (item) {
                    actor->addItem(item);
                    actor->tryEquipItem(item);
                }
                break;
            } else roll -= line.spawnChance;
        }
    }

    return actor;
}

Actor::Actor(const ActorData &data, unsigned myIdent)
: data(data), ident(myIdent), position(-1, -1),
  isPlayer(false), xp(0), playerLastSeenPosition(-1, -1),
  onMap(nullptr)
{ }

Actor::~Actor() {
    for (Item *item : inventory) {
        delete item;
    }
}

std::string Actor::getName(bool definitive) const {
    if (isPlayer) return "you";
    if (definitive) return "the " + data.name;
    else            return "a " + data.name;
}

void Actor::reset() {
    health = getStat(STAT_HEALTH);
    energy = getStat(STAT_ENERGY);
}

int Actor::getStat(int statNumber) const {
    int itemBonus = 0;

    // current bulk and XP can never be boosted
    if (statNumber != STAT_BULK && statNumber != STAT_XP) {
        for (const Item *item : inventory) {
            if (!item->isEquipped) continue; // items only provide static bonuses when equipped
            itemBonus += item->getStatBonus(statNumber);
        }
        for (const StatusItem *status : statusEffects) {
            if (!status) continue;
            for (const EffectData &data : status->data.effects) {
                if (data.trigger != ET_BOOST) continue;
                if (data.effectId == statNumber) {
                    itemBonus += data.effectStrength;
                }
            }

        }
    }

    switch(statNumber) {
        case STAT_STRENGTH:
        case STAT_DEXTERITY:
        case STAT_AGILITY:
        case STAT_TOUGHNESS:
            return itemBonus + data.baseStats[statNumber];
        case STAT_XP:           return xp;
        case STAT_HEALTH:       return 20 + itemBonus + getStat(STAT_TOUGHNESS) * 4;
        case STAT_ENERGY:       return 20 + itemBonus + getStat(STAT_TOUGHNESS) * 2;
        case STAT_TO_HIT:       return itemBonus + getStat(STAT_AGILITY);
        case STAT_EVASION:      return 10 + itemBonus + getStat(STAT_AGILITY);
        case STAT_BULK_MAX:     return 10 + itemBonus + getStat(STAT_STRENGTH) * 2;
        case STAT_BULK: {
            int total = 0;
            for (const Item *item : inventory) {
                if (!item) continue;
                total += item->data.bulk;
            }
            return total;
            break; }
        default:
            return 0;
    }
}

void Actor::takeDamage(int amount) {
    health -= amount;
    if (health < 0) health = 0;
    int maxHealth = getStat(STAT_HEALTH);
    if (health > maxHealth) health = maxHealth;
}

void Actor::spendEnergy(int amount) {
    energy -= amount;
    if (energy < 0) energy = 0;
    int maxEnergy = getStat(STAT_ENERGY);
    if (energy > maxEnergy) energy = maxEnergy;
}

bool Actor::isOverBurdened() const {
    return getStat(STAT_BULK) > getStat(STAT_BULK_MAX);
}

void Actor::addItem(Item *item) {
    if (item) inventory.push_back(item);
}

void Actor::removeItem(Item *item) {
    if (!item) return;
    auto iter = inventory.begin();
    while (iter != inventory.end()) {
        if (*iter == item) {
            inventory.erase(iter);
            return;
        }
        ++iter;
    }
}

void Actor::dropAllItems() {
    // ensure the actor is actually on a map
    if (!onMap || !onMap->isValidPosition(position)) return;
    if (onMap->actorAt(position) != this) return;

    // drop stuff
    for (Item *item : inventory) {
        item->isEquipped = false;
        onMap->addItem(item, position);
    }
    inventory.clear();
}

bool Actor::tryEquipItem(Item *item) {
    if (!item) return false;
    // ensure the actor possesses the item
    bool foundItem = false;
    const Item *oldWeapon = nullptr;
    int talismanCount = 0;
    for (const Item *oldItem : inventory) {
        if (oldItem->data.type == ItemData::Talisman && oldItem->isEquipped) ++talismanCount;
        if (oldItem->data.type == ItemData::Weapon && oldItem->isEquipped) oldWeapon = oldItem;
        if (oldItem == item) foundItem = true;
    }
    if (!foundItem) return false;
    if (item->data.type == ItemData::Talisman) {
        if (talismanCount < MAX_TALISMANS_WORN) {
            item->isEquipped = true;
        } else return false;
    } else if (item->data.type == ItemData::Weapon) {
        if (!oldWeapon) item->isEquipped = true;
        else return false;
    }
    return false;
}

int Actor::getTalismanCount() const {
    int count = 0;
    for (const Item *item : inventory) {
        if (item && item->data.type == ItemData::Talisman && item->isEquipped) {
            ++count;
        }
    }
    return count;
}

static Item *fistsWeapon = nullptr;
const Item* Actor::getCurrentWeapon() const {
    for (const Item *item : inventory) {
        if (item && item->data.type == ItemData::Weapon && item->isEquipped) {
            return item;
        }
    }
    if (!fistsWeapon) fistsWeapon = new Item(getItemData(SIN_FISTS));
    return fistsWeapon;
}


AttackData Actor::meleeAttack(Actor *target) {
    AttackData data;
    data.weapon = getCurrentWeapon();
    if (data.weapon == nullptr) {
        data.errorMessage = "Weapon was nullptr";
        return data;
    }
    data.roll = 1 + globalRNG.upto(20);
    data.toHit = getStat(STAT_TO_HIT);
    data.evasion = target->getStat(STAT_EVASION);
    if (data.roll + data.toHit > data.evasion) {
        int damageRange = data.weapon->data.maxDamage - data.weapon->data.minDamage + 1;
        if (damageRange < 1) damageRange = 1;
        data.damage = data.weapon->data.minDamage + globalRNG.upto(damageRange);
        target->takeDamage(data.damage);
        if (target->isDead() && !target->isPlayer) {
            for (Item *item : target->inventory) data.drops.push_back(item);
            target->dropAllItems();
            xp += 10;
        }
    }
    return data;
}

void Actor::applyStatus(StatusItem *statusItem) {
    statusEffects.push_back(statusItem);
}


std::string statName(int statNumber) {
    switch(statNumber) {
        case STAT_STRENGTH:     return "strength";
        case STAT_DEXTERITY:    return "dexterity";
        case STAT_AGILITY:      return "agility";
        case STAT_TOUGHNESS:    return "toughness";
        case STAT_HEALTH:       return "max health";
        case STAT_ENERGY:       return "max energy";
        case STAT_TO_HIT:       return "to hit bonus";
        case STAT_EVASION:      return "evasion";
        case STAT_BULK_MAX:     return "max bulk";
        case STAT_BULK:         return "bulk";
        case STAT_XP:           return "XP";
        default: return "unknown stat " + std::to_string(statNumber);
    }
}
