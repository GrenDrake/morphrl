#include <iostream>
#include <sstream>
#include <string>
#include "morph.h"


void doInventory(World &world, bool showFloor);

std::string buildCombatMessage(Actor *attacker, Actor *victim, const AttackData &attackData, bool showCalc) {
    std::stringstream s;
    if (attacker->isPlayer) s << "[color=yellow]You[/color] attack [color=yellow]";
    else                    s << "[color=yellow]" << ucFirst(attacker->getName(true)) << "[/color] attacks [color=yellow]";
    s << victim->getName() << "[/color] with ";
    if (attacker->isPlayer) s << "your";
    else s << "their";
    s << " [color=yellow]";
    if (attackData.weapon) s << attackData.weapon->data.name;
    else s << "(nullptr weapon)";
    s << "[/color]. ";

    if (showCalc) {
        s << "[[attack " << attackData.roll << '+' << attackData.toHit;
        s << " vs evasion " << attackData.evasion << "]] ";
    }

    if (attackData.roll + attackData.toHit > attackData.evasion) {
        if (victim->isPlayer) s << "You take [color=red]";
        else s << ucFirst(victim->getName(true)) << " takes [color=red]";
        s << attackData.damage << "[/color] damage. ";
        s << attackData.effectsMessage;
        if (victim->isDead()) {
            s << (victim->isPlayer ? "You" : "They");
            s << " [color=red]die[/color]! ";
        }
        if (!attackData.drops.empty()) {
            s << "They drop " << makeItemList(attackData.drops, 4) << ". ";
        }
    } else {
        s << "[color=red]Miss[/color]! ";
    }
    return s.str();
}

void tryMeleeAttack(World &world, Direction dir) {
    Actor *actor = world.map->actorAt(world.player->position.shift(dir));
    if (!actor) {
        world.addMessage("No one to attack!");
        return;
    }

    AttackData attackData = world.player->meleeAttack(actor);
    world.addMessage(buildCombatMessage(world.player, actor, attackData, world.showCombatMath));
    world.player->advanceSpeedCounter();
    world.tick();
}

void tryMovePlayer(World &world, Direction dir) {
    bool isOverBurdened = world.player->isOverBurdened();
    if (!isOverBurdened && world.map->tryActorStep(world.player, dir)) {
        const MapTile *tile = world.map->at(world.player->position);
        if (tile && tile->floor == TILE_GRASS && world.player->hasVictoryArtifact()) {
            world.addMessage("[color=green]VICTORY![/color] You have escaped the mutagenic dungeons with the ethereal ore!");
            world.gameState = GameState::Victory;
            return;
        }
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
