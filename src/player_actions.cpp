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

    const Item *weapon = world.player->getCurrentWeapon();
    if (weapon == nullptr) {
        world.addMessage("Weapon was nullptr");
        return;
    }
    int roll = 1 + rand() % 20;
    int toHit = world.player->getStat(STAT_TO_HIT);
    int evasion = actor->getStat(STAT_EVASION);
    std::stringstream s;
    s << "[color=yellow]You[/color] attack [color=yellow]";
    s << actor->getName() << "[/color] with your [color=yellow]";
    s << weapon->data.name << "[/color]. (" << roll << '+' << toHit;
    s << " vs " << evasion << ") ";
    if (roll + toHit > evasion) {
        int damageRange = weapon->data.maxDamage - weapon->data.minDamage + 1;
        if (damageRange < 1) damageRange = 1;
        int damage = weapon->data.minDamage + rand() % damageRange;
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
