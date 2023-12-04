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
        logMessage(LOG_ERROR, "Tried to take item while player outside map.");
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



void debug_addThing(World &world, int thingType) {
    std::string result;
    std::string thingTypeStr = "spawn";
    switch(thingType) {
        case TT_ITEM: thingTypeStr = "item"; break;
        case TT_MUTATION: thingTypeStr = "mutation"; break;
        case TT_STATUS_EFFECT: thingTypeStr = "status effect"; break;
    }
    if (!ui_getString("Add Item", "Enter " + thingTypeStr + " ident", result) || result.empty()) return;

    unsigned ident = 0;
    if (!strToInt(result, ident)) {
        ui_alertBox("Error", "Malformed ident number.");
        return;
    }

    switch(thingType) {
        case TT_ITEM: {
            const ItemData &itemData = getItemData(ident);
            if (itemData.ident == BAD_VALUE) {
                ui_alertBox("Error", "Unknown item ident.");
            } else {
                Item *item = new Item(itemData);
                world.player->addItem(item);
                world.addMessage("[color=cyan]DEBUG[/color] give item " + itemData.name);
            }
            return; }
        case TT_MUTATION: {
            const MutationData &mutationData = getMutationData(ident);
            if (mutationData.ident == BAD_VALUE) {
                ui_alertBox("Error", "Unknown mutation ident.");
            } else {
                MutationItem *mutation = new MutationItem(mutationData);
                world.player->applyMutation(mutation);
                world.addMessage("[color=cyan]DEBUG[/color] give mutation " + mutationData.name);
            }
            return; }
        case TT_STATUS_EFFECT: {
            const StatusData &statusData = getStatusData(ident);
            if (statusData.ident == BAD_VALUE) {
                ui_alertBox("Error", "Unknown status effect ident.");
            } else {
                StatusItem *statusEffect = new StatusItem(statusData);
                world.player->applyStatus(statusEffect);
                world.addMessage("[color=cyan]DEBUG[/color] give status effect " + statusData.name);
            }
            return; }
        default:
            world.addMessage("[color=red]ERROR[/color]  Unknown spawn type " + std::to_string(thingType));
    }
}

void debug_doTeleport(World &world) {
    std::string result;
    if (ui_getString("Teleport", "Enter coordinate pair", result) && !result.empty()) {
        auto parts = explodeOnWhitespace(result);
        int x, y;
        if (parts.size() != 2 || !strToInt(parts[0], x) || !strToInt(parts[1], y)) {
            ui_alertBox("Error", "Malformed coordinates.");
        } else if (!world.map->isValidPosition(Coord(x,y))) {
            ui_alertBox("Error", "Coordinates not on map.");
        } else {
            world.map->moveActor(world.player, Coord(x,y));
            world.map->doActorFOV(world.player);
            world.addMessage("[color=cyan]DEBUG[/color] teleported player player to " + std::to_string(x) + "," + std::to_string(y));
        }
    }
}

void debug_killNeighbours(World &world) {
    Direction d = Direction::North;
    int count = 0;
    do {
        Actor *actor = world.map->actorAt(world.player->position.shift(d));
        if (actor) {
            ++count;
            actor->takeDamage(99999);
        }
        d = rotate45(d);
    } while (d != Direction::North);
    world.addMessage("[color=cyan]DEBUG[/color] Killed " + std::to_string(count) + " actors");
    world.tick();
}
