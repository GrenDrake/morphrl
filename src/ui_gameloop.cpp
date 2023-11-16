#include <iostream>
#include <string>
#include <vector>

#include "BearLibTerminal.h"
#include "morph.h"

void debug_saveMapToPNG(const Dungeon &d, bool showActors);

void tryMeleeAttack(World &world, Direction dir);
void tryMovePlayer(World &world, Direction dir);
void tryPlayerTakeItem(World &world);
Item* doInventory(World &world);
void tryPlayerChangeFloor(World &world);


void showActorInfo(World &world, const Actor *actor) {
    if (!actor) return;

    color_t textColour = color_from_argb(255, 196, 196, 196);
    color_t backColour = color_from_argb(255, 0, 0, 0);
    color_t noartColour = color_from_argb(255, 31, 31, 31);

    const int textWidth = 49;

    Image *actorArt = nullptr;
    if (!actor->data.artFile.empty()) {
        actorArt = loadImage("resources/" + actor->data.artFile);
    }

    bool done = false;
    while (!done) {
        terminal_color(textColour);
        terminal_bkcolor(backColour);
        terminal_clear();

        int nextY = 1;
        std::string actorName = "[color=yellow]" + ucFirst(actor->data.name) + "[/color] ";
        if (actor->isPlayer)  actorName += "([color=green]friendly[/color])";
        else                        actorName += "([color=red]hostile[/color])";
        terminal_print(0, nextY, actorName.c_str());
        nextY += 2;

        std::string healthLine = "Health: [color=red]" + std::to_string(percentOf(actor->health, actor->getStat(STAT_HEALTH))) + "[/color]%";
        terminal_print(0, nextY, healthLine.c_str());
        ++nextY;

        terminal_print_ext(0, 24, textWidth, 1, TK_ALIGN_CENTER, "Press a key to return");
        terminal_print_ext(0, nextY + 1, textWidth, 20, TK_ALIGN_DEFAULT, actor->data.desc.c_str());
        if (actorArt) drawImage(textWidth + 1, 0, actorArt);
        else {
            terminal_bkcolor(noartColour);
            terminal_clear_area(textWidth + 1, 0, 30, 35);
        }
        terminal_refresh();

        int key = terminal_read();
        if (key != TK_MOUSE_MOVE && key != TK_MOUSE_SCROLL) break;
    }

    if (actorArt) delete actorArt;
}

const char *youveNeverSeenThatSpace = "You've never seen that space.";
std::string previewMapSpace(World &world, const Coord &where) {
    if (!world.map->isValidPosition(where)) {
        return youveNeverSeenThatSpace;
    }
    const MapTile *tile = world.map->at(where);
    if (tile) {
        std::stringstream s;
        const TileData &td = getTileData(tile->floor);
        if (!tile->everSeen) {
            return youveNeverSeenThatSpace;
        } else if (tile->isSeen) {
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
        } else {
            s << "You saw: [color=yellow]" << td.name << "[/color].";
        }
        return s.str();
    }
    return youveNeverSeenThatSpace;
}

enum class UIMode {
    Normal, ChooseTile
};
void gameloop(World &world) {
    const color_t black = color_from_argb(255, 0, 0, 0);
    const color_t cursorColour = color_from_argb(255, 127, 127, 127);
    const color_t textColour = color_from_argb(255, 192, 192, 192);
    const color_t healthColour = color_from_argb(255, 255, 127, 127);
    const color_t energyColour = color_from_argb(255, 127, 127, 255);
    const color_t depletedColour = color_from_argb(255, 127, 127, 127);
    const color_t hotZone = color_from_argb(255, 98, 32, 32);
    const color_t hotZoneDark = color_from_argb(255, 49, 16, 16);
    const color_t coldZone = color_from_argb(255, 0, 64, 64);
    const color_t coldZoneDark = color_from_argb(255, 0, 32, 32);

    UIMode uiMode = UIMode::Normal;
    Coord cursorPos;
    while (1) {
        int offsetX = world.player->position.x - 30;
        int offsetY = world.player->position.y - 10;
        terminal_color(textColour);
        terminal_bkcolor(black);
        terminal_clear();

        // show log (we do this first so we can remove any extra bits that would
        // overlap other UI elements)
        unsigned yPos = 24;
        if (uiMode == UIMode::Normal) {
            for (auto iter = world.messages.rbegin(); iter != world.messages.rend(); ++iter) {
                dimensions_t dims = terminal_measure_ext(80, 5, iter->text.c_str());
                if (dims.height > 1) yPos -= dims.height - 1;
                terminal_print_ext(0, yPos, 80, 5, TK_ALIGN_DEFAULT, iter->text.c_str());
                --yPos;
                if (yPos < 20) break;
            }
        } else if (uiMode == UIMode::ChooseTile) {
            const std::string desc = previewMapSpace(world, cursorPos);
            terminal_print_ext(0, 20, 80, 5, TK_ALIGN_DEFAULT, desc.c_str());
            terminal_print(0, 24, "X to finish    ENTER or SPACE to examine creature");
        } else {
            std::cerr << "Unsupported UIMode " << static_cast<int>(uiMode) << " in display\n";
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
                if (uiMode == UIMode::ChooseTile && here == cursorPos) {
                    terminal_bkcolor(cursorColour);
                    terminal_put(x, y, ' ');
                }
                if (!world.map->isValidPosition(here)) continue;
                const MapTile *tile = world.map->at(here);
                const TileData &td = getTileData(world.map->floorAt(here));
                if (!world.disableFOV && !tile->everSeen) continue;
                bool isVisible = world.disableFOV || tile->isSeen;
                Actor *actor = world.map->actorAt(here);
                Item *item = world.map->itemAt(here);
                if (uiMode == UIMode::ChooseTile && here == cursorPos) {
                    ;
                } else if (td.isOpaque || tile->temperature == 0) {
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

        // talisman count
        terminal_printf(61, 14, "Talismans: %d", world.player->getTalismanCount());
        // (debug) position data
        terminal_print(61, 16, "Position:");
        terminal_print(63, 17, (std::to_string(world.player->position.x) +
                                ", " +
                                std::to_string(world.player->position.y)).c_str());
        terminal_print(61, 18, ("Turn: " + std::to_string(world.currentTurn)).c_str());

        terminal_refresh();


        int key = terminal_read();
        if (key == TK_CLOSE)    break;
        if (uiMode == UIMode::Normal) {
            if (key == TK_ESCAPE)   break;
            if (key == TK_RIGHT || key == TK_KP_6)  tryMovePlayer(world, Direction::East);
            if (key == TK_LEFT || key == TK_KP_4)   tryMovePlayer(world, Direction::West);
            if (key == TK_DOWN || key == TK_KP_2)   tryMovePlayer(world, Direction::South);
            if (key == TK_UP || key == TK_KP_8)     tryMovePlayer(world, Direction::North);
            if (key == TK_KP_7)                     tryMovePlayer(world, Direction::Northwest);
            if (key == TK_KP_9)                     tryMovePlayer(world, Direction::Northeast);
            if (key == TK_KP_1)                     tryMovePlayer(world, Direction::Southwest);
            if (key == TK_KP_3)                     tryMovePlayer(world, Direction::Southeast);
            if (key == TK_SPACE || key == TK_KP_5)  world.tick();

            if (key == TK_G)        tryPlayerTakeItem(world);
            if (key == TK_COMMA)    tryPlayerChangeFloor(world);
            if (key == TK_PERIOD)   tryPlayerChangeFloor(world);

            if (key == TK_MOUSE_RIGHT) {
                    int mx = terminal_state(TK_MOUSE_X);
                    int my = terminal_state(TK_MOUSE_Y);
                    if (mx < 60 && my < 20) {
                        Coord where(mx + offsetX, my + offsetY);
                        world.addMessage(previewMapSpace(world, where));
                    }
            }

            if (key == TK_I)        doInventory(world);
            if (key == TK_X) {
                uiMode = UIMode::ChooseTile;
                cursorPos = world.player->position;
            }


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
        } else if (uiMode == UIMode::ChooseTile) {
            if (key == TK_MOUSE_LEFT) {
                int mx = terminal_state(TK_MOUSE_X);
                int my = terminal_state(TK_MOUSE_Y);
                if (mx < 60 && my < 20) {
                    cursorPos.x = mx + offsetX;
                    cursorPos.y = my + offsetY;
                }
            }
            if (key == TK_ESCAPE || key == TK_X || key == TK_MOUSE_RIGHT) {
                uiMode = UIMode::Normal;
            }
            if (key == TK_ENTER || key == TK_SPACE || key == TK_KP_ENTER) {
                uiMode = UIMode::Normal;
                const Actor *actor = world.map->actorAt(cursorPos);
                if (actor) showActorInfo(world, actor);
            }
            if (key == TK_RIGHT)    cursorPos = cursorPos.shift(Direction::East);
            if (key == TK_LEFT)     cursorPos = cursorPos.shift(Direction::West);
            if (key == TK_DOWN)     cursorPos = cursorPos.shift(Direction::South);
            if (key == TK_UP)       cursorPos = cursorPos.shift(Direction::North);
        } else {
            std::cerr << "unhandled UIMode " << static_cast<int>(uiMode) << " in input \n";
        }
    }

    return;
}