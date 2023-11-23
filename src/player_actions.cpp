#include <iostream>
#include <sstream>
#include <string>
#include "morph.h"


void doInventory(World &world, bool showFloor);


void tryMeleeAttack(World &world, Direction dir) {
    Actor *actor = world.map->actorAt(world.player->position.shift(dir));
    if (!actor) {
        world.addMessage("No one to attack!");
        return;
    }

    AttackData attackData = world.player->meleeAttack(actor);
    std::stringstream s;
    s << "[color=yellow]You[/color] attack [color=yellow]";
    s << actor->getName() << "[/color] with your [color=yellow]";
    if (attackData.weapon) s << attackData.weapon->data.name;
    else s << "(nullptr weapon)";
    s << "[/color]. (" << attackData.roll << '+' << attackData.toHit;
    s << " vs " << attackData.evasion << ") ";
    if (attackData.roll + attackData.toHit > attackData.evasion) {
        s << ucFirst(actor->getName(true)) << " takes [color=red]";
        s << attackData.damage << "[/color] damage";
        if (actor->isDead()) {
            s << " and [color=red]dies[/color]";
        }
        s << '!';
        if (!attackData.drops.empty()) {
            s << " They drop " << makeItemList(attackData.drops, 4) << '.';
        }
        world.addMessage(s.str());
    } else {
        s << "[color=red]Miss[/color]!";
        world.addMessage(s.str());
    }
    world.player->advanceSpeedCounter();
    world.tick();
}

void tryMovePlayer(World &world, Direction dir) {
    bool isOverBurdened = world.player->isOverBurdened();
    if (!isOverBurdened && world.map->tryActorStep(world.player, dir)) {
        const MapTile *tile = world.map->at(world.player->position);
        if (tile && !tile->items.empty()) {
            std::stringstream s;
            if (tile->items.size() == 1) s << "Item here: ";
            else s << "Items here: ";
            s << makeItemList(tile->items, 4) << '.';
            world.addMessage(s.str());
        }
        world.player->advanceSpeedCounter();
        world.tick();
        return;
    }

    Actor *actor = world.map->actorAt(world.player->position.shift(dir));
    if (actor) tryMeleeAttack(world, dir);
    else if (isOverBurdened) {
        world.addMessage("You are too over-burdened to move!");
    }
}

void tryPlayerTakeItem(World &world) {
    MapTile *tile = world.map->at(world.player->position);
    if (!tile) {
        std::cerr << "Tried to take item while player outside map.\n";
        return;
    }

    if (tile->items.empty()) {
        world.addMessage("Nothing to take!");
    } else if (tile->items.size() == 1) {
        Item *item = tile->items[0];
        world.map->removeItem(item);
        world.player->addItem(item);
        world.player->advanceSpeedCounter();
        world.addMessage("Took [color=yellow]" + item->getName(true) + "[/color].");
        world.tick();
    } else {
        doInventory(world, true);
    }
}

void tryPlayerChangeFloor(World &world) {
    const TileData &td = getTileData(world.map->floorAt(world.player->position));
    if (td.isUpStair) {
        world.movePlayerToDepth(world.map->depth() - 1, DE_UPSTAIRS);
        world.addMessage("Ascending to " + world.map->data.name + ".");
    } else if (td.isDownStair) {
        world.movePlayerToDepth(world.map->depth() + 1, DE_DOWNSTAIRS);
        world.addMessage("Descending to " + world.map->data.name + ".");
    } else {
        world.addMessage("No stairs here!");
    }
}
