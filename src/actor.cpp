#include <algorithm>
#include <iostream>
#include <vector>

#include "morph.h"


StatusItem::StatusItem(const StatusData &data)
: data(data), duration(0)
{ }

MutationItem::MutationItem(const MutationData &data)
: data(data)
{ }


std::vector<unsigned> getIdentsForSpawn(const std::vector<SpawnLine> &spawnLines, bool processGroup0) {
    std::vector<unsigned> toSpawn;
    std::vector<int> spawnGroups;

    for (const SpawnLine &line : spawnLines) {
        if (line.spawnGroup == 0) {
            int roll = globalRNG.upto(100);
            if (roll < line.spawnChance) {
                toSpawn.push_back(line.ident);
            }
        } else {
            addUniqueToVector(spawnGroups, line.spawnGroup);
        }
    }

    for (int groupId : spawnGroups) {
        int roll = globalRNG.upto(100);
        for (const SpawnLine &line : spawnLines) {
            if (line.spawnGroup != groupId) continue;
            if (roll < line.spawnChance) {
                toSpawn.push_back(line.ident);
                break;
            } else roll -= line.spawnChance;
        }
    }

    return toSpawn;
}


Actor* Actor::create(const ActorData &data) {
    if (data.ident == BAD_VALUE) return nullptr;
    Actor *actor = new Actor(data, 0);
    if (!actor) return nullptr;

    std::vector<unsigned> toSpawn = getIdentsForSpawn(data.initialItems, true);
    for (unsigned ident : toSpawn) {
        Item *item = new Item(getItemData(ident));
        if (item) {
            actor->addItem(item);
            actor->tryEquipItem(item);
        }
    }

    std::vector<unsigned> mutations = getIdentsForSpawn(data.initialMutations, true);
    for (unsigned ident : mutations) {
        MutationItem *mutation = new MutationItem(getMutationData(ident));
        actor->mutations.push_back(mutation);
    }

    return actor;
}

Actor::Actor(const ActorData &data, unsigned myIdent)
: data(data), ident(myIdent), position(-1, -1),
  isPlayer(false), level(0), xp(0), advancementPoints(0), playerLastSeenPosition(-1, -1),
  speedCounter(0), onMap(nullptr), turnsSinceCombatAction(0)
{
    level = data.baseLevel;
    for (int i = 0; i < STAT_BASE_COUNT; ++i) {
    statLevels[i] = 0;
    }
}

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

void Actor::verify() {
    if (health < 0) health = 0;
    if (health > getStat(STAT_HEALTH)) health = getStat(STAT_HEALTH);
}

int Actor::getStatLevelBonus(int statNumber) const {
    if (statNumber >= STAT_BASE_COUNT) return 0;
    return statLevels[statNumber];
}

int Actor::getStatStatusBonus(int statNumber) const {
    if (statNumber == STAT_BULK) return 0;
    int bonus = 0;
    for (const StatusItem *status : statusEffects) {
        if (!status) continue;
        for (const EffectData &data : status->data.effects) {
            if (data.trigger != ET_BOOST) continue;
            if (data.effectId == statNumber) {
                bonus += data.effectStrength;
            }
        }
    }
    return bonus;
}

int Actor::getStatMutationBonus(int statNumber) const {
    if (statNumber == STAT_BULK) return 0;
    int bonus = 0;
    for (const MutationItem *mutation : mutations) {
        if (!mutation) continue;
        for (const EffectData &data : mutation->data.effects) {
            if (data.trigger != ET_BOOST) continue;
            if (data.effectId == statNumber) {
                bonus += data.effectStrength;
            }
        }
    }
    // add the bonus from any mutation-granted unarmed attack
    const Item *weapon = getCurrentWeapon();
    if (!weapon || weapon->isEquipped) return bonus;
    for (const EffectData &data : weapon->data.effects) {
        if (data.trigger != ET_BOOST) continue;
        if (data.effectId == statNumber) {
            bonus += data.effectStrength;
        }
    }
    return bonus;
}

int Actor::getStatItemBonus(int statNumber) const {
    if (statNumber == STAT_BULK) return 0;
    int bonus = 0;
    for (const Item *item : inventory) {
        if (!item->isEquipped) continue; // items only provide static bonuses when equipped
        bonus += item->getStatBonus(statNumber);
    }
    return bonus;
}

int Actor::getStatBase(int statNumber) const {
    switch(statNumber) {
        case STAT_STRENGTH:
        case STAT_SPEED:
        case STAT_ACCURACY:
        case STAT_TOUGHNESS:    return data.baseStats[statNumber];
        case STAT_EVASION:      return 10 + data.baseStats[statNumber];

        case STAT_HEALTH:       return 20 + getStat(STAT_TOUGHNESS) * 4;
        case STAT_ENERGY:       return 20 + getStat(STAT_TOUGHNESS) * 2;
        case STAT_BULK_MAX:     return 10 + getStat(STAT_STRENGTH) * 2;
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

int Actor::getStat(int statNumber) const {
    int bonus = 0;

    // current bulk can never be boosted
    if (statNumber != STAT_BULK) {
        bonus += getStatLevelBonus(statNumber);
        bonus += getStatItemBonus(statNumber);
        bonus += getStatStatusBonus(statNumber);
        bonus += getStatMutationBonus(statNumber);
    }
    return bonus += getStatBase(statNumber);
}

void Actor::takeDamage(int amount, Actor *fromWho) {
    if (amount > 0) turnsSinceCombatAction = 0;
    health -= amount;
    if (health <= 0) {
        health = 0;
        if (fromWho) {
            int levelDiff = level - fromWho->level;
            int xpGain = 10 + levelDiff * 2;
            if (xpGain > 0) fromWho->giveXP(xpGain);
        }
    }
    int maxHealth = getStat(STAT_HEALTH);
    if (health > maxHealth) health = maxHealth;
}

void Actor::spendEnergy(int amount) {
    energy -= amount;
    if (energy < 0) energy = 0;
    int maxEnergy = getStat(STAT_ENERGY);
    if (energy > maxEnergy) energy = maxEnergy;
}

void Actor::giveXP(int amount) {
    xp += amount;
    while (xp >= XP_PER_LEVEL) {
        xp -= XP_PER_LEVEL;
        ++advancementPoints;
        ++level;
    }
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

bool Actor::hasVictoryArtifact() const {
    for (const Item *item : inventory) {
        if (item && item->data.isVictoryArtifact) return true;
    }
    return false;
}

static std::vector<Item*> unarmedWeapons;
const Item* Actor::getCurrentWeapon() const {
    // first check if the actor is wielding a weapon; if so, just return that one
    for (const Item *item : inventory) {
        if (item && item->data.type == ItemData::Weapon && item->isEquipped) {
            return item;
        }
    }

    // check for any mutation granted unarmed attacks
    unsigned weaponIdent = BAD_VALUE;
    for (const MutationItem *mutation : mutations) {
        for (const EffectData &data : mutation->data.effects) {
            if (data.trigger == ET_UNARMED_ATTACK) {
                weaponIdent = data.effectId;
            }
        }
    }
    // otherwise just use the generic fists weapon
    if (weaponIdent == BAD_VALUE) weaponIdent = SIN_FISTS;

    // retrieve (or create) and return the item for this weapon
    for (Item *item : unarmedWeapons) {
        if (item && item->data.ident == weaponIdent) return item;
    }
    Item *newWeapon = new Item(getItemData(weaponIdent));
    unarmedWeapons.push_back(newWeapon);
    return newWeapon;
}

std::string Actor::triggerOnHitEffects(Actor *target, const Item *weapon) {
    std::string result;
    if (!weapon) weapon = getCurrentWeapon();
    if (weapon && !weapon->isEquipped) {
        for (const EffectData &data : weapon->data.effects) {
            if (data.trigger == ET_ON_HIT) {
                result += triggerEffect(data, this, target);
            }
        }
    }
    for (const Item *item : inventory) {
        if (!item || !item->isEquipped) continue;
        for (const EffectData &data : item->data.effects) {
            if (data.trigger == ET_ON_HIT) {
                result += triggerEffect(data, this, target);
            }
        }
    }
    return result;
}

AttackData Actor::meleeAttackWithWeapon(Actor *target, const Item *weapon) {
    AttackData data;
    if (weapon) data.weapon = weapon;
    else data.weapon = getCurrentWeapon();
    if (!target) {
        data.errorMessage = "meleeAttackWithWeapon: target was nullptr";
        return data;
    }
    if (data.weapon == nullptr) {
        data.errorMessage = "meleeAttackWithWeapon: weapon was nullptr";
        return data;
    }

    turnsSinceCombatAction = 0;
    target->turnsSinceCombatAction = 0;
    if (target->data.isFragile) {
        target->takeDamage(9999, this);
        data.effectsMessage = triggerOnHitEffects(target, data.weapon);
        if (target->isDead() && !target->isPlayer) {
            for (Item *item : target->inventory) data.drops.push_back(item);
            target->dropAllItems();
        }
        data.roll = 20;
        data.toHit = 0;
        data.evasion = 0;
        data.damageMin = 9999;
        data.damageMax = 9999;
        data.damageBonus = 0;
        data.damage = 9999;

    } else {
        data.roll = 1 + globalRNG.upto(20);
        data.toHit = getStat(STAT_ACCURACY);
        data.evasion = target->getStat(STAT_EVASION);
        int damageBonus = getStat(STAT_DAMAGE_BONUS);

        if (weapon) {
            // attacking with specific weapon (for ability-based attacks, etc.)
            const Item *actualWeapon = getCurrentWeapon();
            // remove normal weapon stats
            data.toHit -= actualWeapon->getStatBonus(STAT_ACCURACY);
            damageBonus -= actualWeapon->getStatBonus(STAT_DAMAGE_BONUS);
            // add special weapon stats
            data.toHit += weapon->getStatBonus(STAT_ACCURACY);
            damageBonus += weapon->getStatBonus(STAT_DAMAGE_BONUS);
        }

        if (data.roll + data.toHit > data.evasion) {
            int damageRange = data.weapon->data.maxDamage - data.weapon->data.minDamage + 1;
            if (damageRange < 1) damageRange = 1;
            data.damageMin = data.weapon->data.minDamage;
            data.damageMax = data.weapon->data.maxDamage;
            data.damageBonus = getStat(STAT_STRENGTH) + damageBonus;
            data.damage = data.weapon->data.minDamage + globalRNG.upto(damageRange) + getStat(STAT_STRENGTH);
            data.damage += damageBonus;
            if (data.damage < 1) data.damage = 1;
            target->takeDamage(data.damage, this);
            data.effectsMessage = triggerOnHitEffects(target, data.weapon);
            if (target->isDead() && !target->isPlayer) {
                for (Item *item : target->inventory) data.drops.push_back(item);
                target->dropAllItems();
            }
        }
    }
    return data;
}

AttackData Actor::meleeAttack(Actor *target) {
    return meleeAttackWithWeapon(target, nullptr);
}

void Actor::advanceSpeedCounter(int multiplier) {
    int turnTime = getStat(STAT_SPEED) * -2 + 10;
    turnTime = turnTime * multiplier / 100;
    speedCounter += turnTime;
}

MutationItem* Actor::mutationForSlot(unsigned slotNumber) {
    for (MutationItem *item : mutations) {
        if (item && item->data.slot == slotNumber) return item;
    }
    return nullptr;
}

bool Actor::hasMutation(unsigned mutationIdent) const {
    for (const MutationItem *mut : mutations) {
        if (mut && mut->data.ident == mutationIdent) return true;
    }
    return false;
}

bool mutationSort(const MutationItem *lhs, const MutationItem *rhs) {
    if (lhs->data.slot < rhs->data.slot) return true;
    if (lhs->data.slot > rhs->data.slot) return false;
    if (lhs->data.name < rhs->data.name) return true;
    if (lhs->data.name > rhs->data.name) return false;
    return false;
}
void Actor::applyMutation(MutationItem *mutation) {
    if (!mutation) return;
    if (hasMutation(mutation->data.ident)) {
        delete mutation;
        return;
    }
    if (mutation->data.slot != 0) {
        MutationItem *old = mutationForSlot(mutation->data.slot);
        if (old) removeMutation(old);
        delete old;
    }
    if (!mutation->data.isNonMutation()) {
        mutations.push_back(mutation);
        std::sort(mutations.begin(), mutations.end(), mutationSort);
    }
}

void Actor::removeMutation(MutationItem *mutation) {
    auto iter = mutations.begin();
    while (iter != mutations.end()) {
        if (*iter == mutation) {
            mutations.erase(iter);
            return;
        }
        ++iter;
    }
}

void Actor::applyStatus(StatusItem *statusItem) {
    statusEffects.push_back(statusItem);
}

bool Actor::hasStatus(unsigned statusIdent) const {
    for (const StatusItem *status : statusEffects) {
        if (status && status->data.ident == statusIdent) return true;
    }
    return false;
}

void addAbilitiesFromEffects(const std::vector<EffectData> &effects, std::vector<unsigned> &result) {
    for (const EffectData &e : effects) {
        if (e.trigger != ET_GIVE_ABILITY) continue;
        result.push_back(e.effectId);
    }
}
std::vector<unsigned> Actor::getAbilityList() const {
    std::vector<unsigned> result;
    for (const MutationItem *mutation : mutations) {
        addAbilitiesFromEffects(mutation->data.effects, result);
    }
    return result;
}


std::string statName(int statNumber) {
    switch(statNumber) {
        case STAT_STRENGTH:     return "strength";
        case STAT_SPEED:        return "speed";
        case STAT_EVASION:      return "evasion";
        case STAT_ACCURACY:     return "accuracy";
        case STAT_TOUGHNESS:    return "toughness";
        case STAT_HEALTH:       return "max health";
        case STAT_ENERGY:       return "max energy";
        case STAT_BULK_MAX:     return "max bulk";
        case STAT_DAMAGE_BONUS: return "damage bonus";
        case STAT_BULK:         return "bulk";
        default: return "unknown stat " + std::to_string(statNumber);
    }
}
