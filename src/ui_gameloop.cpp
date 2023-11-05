#include <iostream>
#include <string>
#include <vector>

#include "BearLibTerminal.h"
#include "morph.h"

void debug_saveMapToPNG(const Dungeon &d, bool showActors);

void tryMeleeAttack(World &world, Direction dir);
void tryMovePlayer(World &world, Direction dir);
void tryPlayerTakeItem(World &world);
void tryPlayerDropItem(World &world);
void tryPlayerUseItem(World &world);
void tryPlayerChangeFloor(World &world);


void examineMapSpace(World &world, const Coord &where) {
    const MapTile *tile = world.map->at(where);
    if (!tile) return;
    const TileData &td = getTileData(tile->floor);

    color_t textColour = color_from_argb(255, 196, 196, 196);
    color_t backColour = color_from_argb(255, 0, 0, 0);
    color_t highlight = color_from_name("yellow");
    const int textWidth = 50;
    const int column2x = textWidth + 2;

    bool done = false;
    while (!done) {
        terminal_color(textColour);
        terminal_bkcolor(backColour);
        terminal_clear();
        terminal_print(0, 0, "You see:");

        int nextY = 2;
        std::string tileName = "[color=yellow]" + ucFirst(td.name) + "[/color]";
        if (tile->temperature < 0) tileName += " ([color=cyan]cold[/color] region)";
        else if (tile->temperature > 0) tileName += " ([color=red]hot[/color] region)";
        // terminal_color(highlight);
        terminal_print(0, nextY, tileName.c_str());
        // terminal_color(textColour);
        dimensions_t printedSize = terminal_print_ext(0, nextY + 1, textWidth, 5, TK_ALIGN_DEFAULT, td.desc.c_str());
        nextY += printedSize.height + 2;

        if (tile->actor) {
            std::string actorName = "[color=yellow]" + ucFirst(tile->actor->data.name) + "[/color] ";
            if (tile->actor->isPlayer)  actorName += "([color=green]friendly[/color])";
            else                        actorName += "([color=red]hostile[/color])";
            terminal_color(highlight);
            terminal_print(0, nextY, actorName.c_str());
            terminal_color(textColour);
            dimensions_t printedSize = terminal_print_ext(0, nextY+1, textWidth, 5, TK_ALIGN_DEFAULT, tile->actor->data.desc.c_str());
            std::string healthLine = "Health: " + std::to_string(percentOf(tile->actor->health, tile->actor->getStat(STAT_HEALTH))) + "%";

            terminal_print(column2x, nextY + 1, healthLine.c_str());
            if (printedSize.height < 1) printedSize.height = 1;
            nextY += printedSize.height + 2;
        }

        // if (!tile->items.empty()) {
            // terminal_color(highlight);
            // terminal_print(0, nextY, ucFirst(tile->item->getName(true)).c_str());
            // terminal_color(textColour);
            // dimensions_t printedSize = terminal_print_ext(0, nextY+1, textWidth, 5, TK_ALIGN_DEFAULT, tile->item->data.desc.c_str());
            // nextY += printedSize.height + 2;
        // }
        terminal_refresh();

        int key = terminal_read();
        if (key != TK_MOUSE_MOVE && key != TK_MOUSE_SCROLL) break;
    }
}

void previewMapSpace(World &world, const Coord &where) {
    const MapTile *tile = world.map->at(where);
    if (tile) {
        std::stringstream s;
        const TileData &td = getTileData(tile->floor);
        s << "You see: [color=yellow]" << td.name << "[/color]";
        if (tile->temperature < 0) s << " ([color=cyan]cold[/color] area)";
        if (tile->temperature > 0) s << " ([color=red]hot[/color] area)";
        if (tile->actor || !tile->items.empty()) s << " containing";
        if (tile->actor) {
            s << " [color=yellow]" << tile->actor->getName() << "[/color] (";
            s << percentOf(tile->actor->health, tile->actor->getStat(STAT_HEALTH));
            s << "%)";
        }
        if (tile->actor && !tile->items.empty()) s << " and";
        if (!tile->items.empty()) {
            if (tile->items.size() > 4) {
                s << " many items";
            } else if (tile->items.size() == 1) {
                    s << " [color=yellow]" << tile->items.front()->getName() << "[/color]";
            } else {
                for (unsigned i = 0; i < tile->items.size(); ++i) {
                    if (i == tile->items.size() - 1) s << " and";
                    else if (i > 0) s << ",";
                    s << " [color=yellow]" << tile->items[i]->getName() << "[/color]";
                }
            }
        }
        s << '.';
        world.addMessage(s.str());
    }
}

void handleMouseClick(World &world, int offsetX, int offsetY, int button) {
    int mx = terminal_state(TK_MOUSE_X);
    int my = terminal_state(TK_MOUSE_Y);
    if (mx < 60 && my < 20) {
        Coord where(mx + offsetX, my + offsetY);
        if (button == 0) previewMapSpace(world, where);
        else if (button == 1) examineMapSpace(world, where);
    }
}


void gameloop(World &world) {
    const color_t black = color_from_argb(255, 0, 0, 0);
    const color_t textColour = color_from_argb(255, 192, 192, 192);
    const color_t healthColour = color_from_argb(255, 255, 127, 127);
    const color_t energyColour = color_from_argb(255, 127, 127, 255);
    const color_t depletedColour = color_from_argb(255, 127, 127, 127);
    const color_t hotZone = color_from_argb(255, 98, 32, 32);
    const color_t hotZoneDark = color_from_argb(255, 49, 16, 16);
    const color_t coldZone = color_from_argb(255, 0, 64, 64);
    const color_t coldZoneDark = color_from_argb(255, 0, 32, 32);

    while (1) {
        int offsetX = world.player->position.x - 30;
        int offsetY = world.player->position.y - 10;
        terminal_color(textColour);
        terminal_bkcolor(black);
        terminal_clear();

        // show log (we do this first so we can remove any extra bits that would
        // overlap other UI elements)
        unsigned yPos = 24;
        for (auto iter = world.messages.rbegin(); iter != world.messages.rend(); ++iter) {
            dimensions_t dims = terminal_measure_ext(80, 5, iter->text.c_str());
            if (dims.height > 1) yPos -= dims.height - 1;
            terminal_print_ext(0, yPos, 80, 5, TK_ALIGN_DEFAULT, iter->text.c_str());
            --yPos;
            if (yPos < 20) break;
        }
        terminal_clear_area(0, 0, 80, 20);

        // draw interface frame
        for (int x = 0; x < 80; ++x) {
            terminal_put(x, 19, 0x2500);
        }
        for (int y = 0; y < 19; ++y) {
            terminal_put(60, y, 0x2502);
        }
        terminal_put(60, 19, 0x2534);

        // draw map
        for (int y = 0; y < 19; ++y) {
            for (int x = 0; x < 60; ++x) {
                Coord here(Coord(offsetX + x, offsetY + y));
                if (!world.map->isValidPosition(here)) continue;
                const MapTile *tile = world.map->at(here);
                const TileData &td = getTileData(world.map->floorAt(here));
                if (!world.disableFOV && !tile->everSeen) continue;
                bool isVisible = world.disableFOV || tile->isSeen;
                Actor *actor = world.map->actorAt(here);
                Item *item = world.map->itemAt(here);
                if (td.isOpaque || tile->temperature == 0) {
                    terminal_bkcolor(black);
                } else if (tile->temperature < 0) {
                    if (isVisible) terminal_bkcolor(coldZone);
                    else           terminal_bkcolor(coldZoneDark);
                } else { // if (tile->temperature > 0) {
                    if (isVisible) terminal_bkcolor(hotZone);
                    else           terminal_bkcolor(hotZoneDark);
                }
                if (isVisible && actor) {
                    terminal_color(color_from_argb(255,
                                                   actor->data.r,
                                                   actor->data.g,
                                                   actor->data.b));
                    terminal_put(x, y, actor->data.glyph);
                } else if (isVisible && item) {
                    terminal_color(color_from_argb(255,
                                                   item->data.r,
                                                   item->data.g,
                                                   item->data.b));
                    terminal_put(x, y, item->data.glyph);
                } else {
                    if (isVisible) {
                        terminal_color(color_from_argb(255, td.r, td.g, td.b));
                    } else {
                        terminal_color(color_from_argb(255, td.r / 2, td.g / 2, td.b / 2));
                    }
                    terminal_put(x, y, td.glyph);
                }
            }
        }

        // ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
        // health and energy
        terminal_bkcolor(healthColour);
        int healthPercent = 0;
        if (world.player->health < 1) healthPercent = 0;
        else {
            healthPercent = percentOf(world.player->health,
                                      world.player->getStat(STAT_HEALTH)) / 10;
            if (world.player->health > 0 && healthPercent < 1) healthPercent = 1;
        }
        for (int i = 0; i < 10; ++i) {
            if (i >= healthPercent) terminal_bkcolor(depletedColour);
            terminal_put(61+i, 2, ' ');
        }
        terminal_bkcolor(energyColour);
        int energyPercent = 0;
        if (world.player->energy < 1) energyPercent = 0;
        else {
            energyPercent = percentOf(world.player->energy,
                                      world.player->getStat(STAT_ENERGY)) / 10;
            if (world.player->energy > 0 && energyPercent < 1) energyPercent = 1;
        }
        for (int i = 0; i < 10; ++i) {
            if (i >= energyPercent) terminal_bkcolor(depletedColour);
            terminal_put(61+i, 3, ' ');
        }
        terminal_bkcolor(black);

        // player name & ID
        terminal_color(textColour);
        terminal_print(61, 0, "Player the Player");
        terminal_print(72, 2, std::to_string(world.player->health).c_str());
        terminal_print(72, 3, std::to_string(world.player->energy).c_str());

        // stats
        for (int i = 0; i < STAT_EXTRA_COUNT; ++i) {
            int yPos = 5 + i;
            if (i >= STAT_BASE_COUNT) ++yPos;
            std::string text = statName(i) + ": " + std::to_string(world.player->getStat(i));
            terminal_print(61, yPos, text.c_str());
        }

        // weapon and talisman count
        const Item *currentWeapon = world.player->getCurrentWeapon();
        if (currentWeapon) {
            terminal_print(61, 13, currentWeapon->getName().c_str());
        }
        terminal_printf(61, 14, "Talismans: %d", world.player->getTalismanCount());
        // (debug) position data
        terminal_print(61, 16, "Position:");
        terminal_print(63, 17, (std::to_string(world.player->position.x) +
                                ", " +
                                std::to_string(world.player->position.y)).c_str());
        terminal_print(61, 18, ("Turn: " + std::to_string(world.currentTurn)).c_str());

        terminal_refresh();


        int key = terminal_read();
        if (key == TK_ESCAPE)   break;
        if (key == TK_CLOSE)    break;
        if (key == TK_RIGHT)    tryMovePlayer(world, Direction::East);
        if (key == TK_LEFT)     tryMovePlayer(world, Direction::West);
        if (key == TK_DOWN)     tryMovePlayer(world, Direction::South);
        if (key == TK_UP)       tryMovePlayer(world, Direction::North);
        if (key == TK_SPACE)    world.tick();
        if (key == TK_G)        tryPlayerTakeItem(world);
        if (key == TK_D)        tryPlayerDropItem(world);
        if (key == TK_COMMA)    tryPlayerChangeFloor(world);
        if (key == TK_PERIOD)   tryPlayerChangeFloor(world);

        if (key == TK_MOUSE_LEFT) handleMouseClick(world, offsetX, offsetY, 0);
        if (key == TK_MOUSE_RIGHT) handleMouseClick(world, offsetX, offsetY, 1);

        if (key == TK_I)        tryPlayerUseItem(world);


        if (key == TK_F10)   debug_saveMapToPNG(*world.map, true);
        if (key == TK_F11)   debug_saveMapToPNG(*world.map, false);
        if (key == TK_F1)    world.addMessage("This is a really long message for testing purposes and so I can see how well word wrap works from messages and so one and so forth and such is what this message is for.");
        if (key == TK_F2)    world.player->takeDamage(1 + rand() % 10);
        if (key == TK_F3)    world.player->spendEnergy(1 + rand() % 10);
        if (key == TK_F4)    world.player->takeDamage(-(1 + rand() % 10));
        if (key == TK_F5)    world.player->spendEnergy(-(1 + rand() % 10));
        if (key == TK_F6)    world.disableFOV = !world.disableFOV;
        if (key == TK_F7) {
            // for (int i = 0; i < 30 && !world.player->isOverBurdened(); ++i) {
            for (int i = 0; i < 30 ; ++i) {
                Item *item = new Item(getItemData(rand()%6));
                world.player->addItem(item);
            }
        }
        if (key == TK_F8)   world.map->calcDistances(world.player->position);
        if (key == TK_F9)   ui_alertBox("Testing", "This is a really long message for testing purposes and so I can see how well word wrap works from messages and so one and so forth and such is what this message is for.");
    }

    return;
}