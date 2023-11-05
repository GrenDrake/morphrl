#include <iostream>
#include <sstream>
#include <string>
#include "morph.h"


Item* selectInventoryItem(World &world, const std::string &prompt);


void tryMeleeAttack(World &world, Direction dir) {
    Actor *actor = world.map->actorAt(world.player->position.shift(dir));
    if (!actor) {
        world.addMessage("No one to attack!");
        return;
    }

    int roll = 1 + rand() % 20;
    int toHit = world.player->getStat(STAT_TO_HIT);
    int evasion = actor->getStat(STAT_EVASION);
    std::stringstream s;
    s << "[color=yellow]You[/color] attack [color=yellow]";
    s << actor->getName() << "[/color]. (" << roll << '+' << toHit;
    s << " vs " << evasion << ") ";
    if (roll + toHit > evasion) {
        int damage = 1 + rand() % 20;
        actor->takeDamage(damage);
        s << "" << ucFirst(actor->getName(true)) << " takes [color=red]";
        s << damage << "[/color] damage";
        if (actor->isDead()) {
            s << " and [color=red]dies[/color]!";
            world.player->xp += 10;
        } else s << '.';
        world.addMessage(s.str());
    } else {
        s << "[color=red]Miss[/color]!";
        world.addMessage(s.str());
    }
    world.tick();
}

void tryMovePlayer(World &world, Direction dir) {
    bool isOverBurdened = world.player->isOverBurdened();
    if (!isOverBurdened && world.map->tryActorStep(world.player, dir)) {
        world.tick();
        // Item *item = world.map->itemAt(world.player->position);
        return;
    }

    Actor *actor = world.map->actorAt(world.player->position.shift(dir));
    if (actor) tryMeleeAttack(world, dir);
    else if (isOverBurdened) {
        world.addMessage("You are too over-burdened to move!");
    }
}

void tryPlayerTakeItem(World &world) {
    Item *item = world.map->itemAt(world.player->position);
    if (!item) {
        world.addMessage("Nothing to take!");
    } else {
        world.map->removeItem(item);
        world.player->addItem(item);
        world.tick();
        world.addMessage("Took [color=yellow]" + item->getName(true) + "[/color].");
    }
}

void tryPlayerDropItem(World &world) {
    Coord dropWhere = world.map->nearestOpenTile(world.player->position);
    
    if (dropWhere.x == -1) {
        world.addMessage("No room to drop item.");
        return;
    }

    Item *item = selectInventoryItem(world, "Drop what?");
    if (!item) return; // cancelled
    world.player->removeItem(item);
    world.map->addItem(item, dropWhere);
    world.addMessage("Dropped [color=yellow]" + item->getName(true) + "[/color].");
    world.tick();
}



void tryPlayerUseItem(World &world) {
    Item *item = selectInventoryItem(world, "Select item...");
    if (!item) return;

    std::stringstream msg;
    switch(item->data.type) {
        case ItemData::Weapon: // equip it
            if (item->isEquipped) {
                item->isEquipped = false;
                msg << "Stopped wielding [color=yellow]" << item->getName(true) << "[/color].";
            } else {
                for (Item *itemIter : world.player->inventory) {
                    if (itemIter && itemIter->data.type == ItemData::Weapon && itemIter->isEquipped) {
                        itemIter->isEquipped = false;
                        msg << "Stopped wielding [color=yellow]";
                        msg << itemIter->getName(true) << "[/color]. ";
                    }
                }
                item->isEquipped = true;
                msg << "Now wielding [color=yellow]" << item->getName(true) << "[/color].";
            }
            world.addMessage(msg.str());
            world.tick();
            return;
        case ItemData::Talisman: // equip it
            if (item->isEquipped) {
                item->isEquipped = false;
                msg << "Stopped wearing [color=yellow]" << item->getName(true) << "[/color].";
            } else {
                int talismanCount = world.player->getTalismanCount();
                if (talismanCount < 3) {
                    item->isEquipped = true;
                    msg << "Now wearing [color=yellow]" << item->getName(true) << "[/color].";
                } else {
                    msg << "You're already wearing the maximum number of talismans; remove one to wear [color=yellow]" << item->getName(true) << "[/color].";
                    world.addMessage(msg.str());
                    return;
                }
            }
            world.addMessage(msg.str());
            world.tick();
            return;
        case ItemData::Consumable: { // activate it
            std::stringstream msg;
            bool didEffect = false;
            msg << "Using [color=yellow]" << ucFirst(item->getName(true)) << "[/color]: ";
            std::cerr << item->data.effects.size() << '\n';
            for (const EffectData &data : item->data.effects) {
                if (data.trigger != ET_ON_USE) continue;
                msg << "Effect " << data.effectId << ". ";
                didEffect = true;
            }

            if (!didEffect) {
                msg << "no effect. ";
            }
            int roll = rand() % 100;
            if (item->data.consumeChance > roll) {
                world.player->removeItem(item);
                delete item;
                msg << "Consumed.";
            }
            world.addMessage(msg.str());
            break; }
        default:
            world.addMessage("[color=yellow]" + ucFirst(item->getName(true)) +
                             "[/color] is not something you can use.");
    }
}

void tryPlayerChangeFloor(World &world) {
    const TileData &td = getTileData(world.map->floorAt(world.player->position));
    if (td.isUpStair) {
        world.addMessage("UP!");
    } else if (td.isDownStair) {
        world.addMessage("DOWN!");
    } else {
        world.addMessage("No stairs here!");
    }
}
