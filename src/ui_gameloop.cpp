#include <iostream>
#include <string>
#include <vector>

#include "BearLibTerminal.h"
#include "morph.h"

void debug_saveMapToPNG(const Dungeon &d, bool showActors);

void showActorInfo(World &world, const Actor *actor);
void doInventory(World &world, bool showFloor);
void doMessageLog(World &world);
void doCharInfo(World &world);

void tryMeleeAttack(World &world, Direction dir);
void tryMovePlayer(World &world, Direction dir);
void tryPlayerTakeItem(World &world);
void tryPlayerChangeFloor(World &world);




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
                s << ' ';
                s << makeItemList(tile->items, 4);
            }
            s << '.';
        } else {
            s << "You saw: [color=yellow]" << td.name << "[/color].";
        }
        return s.str();
    }
    return youveNeverSeenThatSpace;
}

Direction keyToDirection(int key) {
    if (key == TK_SPACE || key == TK_KP_5)  return Direction::Here;
    if (key == TK_RIGHT || key == TK_KP_6)  return Direction::East;
    if (key == TK_LEFT || key == TK_KP_4)   return Direction::West;
    if (key == TK_DOWN || key == TK_KP_2)   return Direction::South;
    if (key == TK_UP || key == TK_KP_8)     return Direction::North;
    if (key == TK_KP_7)                     return Direction::Northwest;
    if (key == TK_KP_9)                     return Direction::Northeast;
    if (key == TK_KP_1)                     return Direction::Southwest;
    if (key == TK_KP_3)                     return Direction::Southeast;
    return Direction::Unknown;
}

enum class UIMode {
    Normal, ExamineTile, ChooseDirection, PlayerDead
};
const int UI_DEBUG_TUNNEL = 10000;
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

    std::string uiModeString;
    int uiModeAction = 0;
    UIMode uiMode = UIMode::Normal;
    Coord cursorPos;
    bool shownDeathMessage = false;
    while (1) {
        if (uiMode == UIMode::Normal && world.player->isDead()) {
            uiMode = UIMode::PlayerDead;
            if (!shownDeathMessage) {
                shownDeathMessage = true;
                world.addMessage("You have [color=red]died[/color]! Press [color=yellow]ESCAPE[/color] to return to the menu.");
            }
        }

        int offsetX = world.player->position.x - 30;
        int offsetY = world.player->position.y - 10;
        terminal_color(textColour);
        terminal_bkcolor(black);
        terminal_clear();

        // show log (we do this first so we can remove any extra bits that would
        // overlap other UI elements)
        unsigned yPos = 24;
        if (uiMode == UIMode::Normal || uiMode == UIMode::PlayerDead) {
            for (auto iter = world.messages.rbegin(); iter != world.messages.rend(); ++iter) {
                dimensions_t dims = terminal_measure_ext(77, 5, iter->text.c_str());
                if (dims.height > 1) yPos -= dims.height - 1;
                terminal_put(0, yPos, '*');
                terminal_print_ext(2, yPos, 77, 5, TK_ALIGN_DEFAULT, iter->text.c_str());
                --yPos;
                if (yPos < 20) break;
            }
        } else if (uiMode == UIMode::ExamineTile) {
            const std::string desc = previewMapSpace(world, cursorPos);
            terminal_print_ext(0, 20, 80, 5, TK_ALIGN_DEFAULT, desc.c_str());
            const Actor *actorAtPos = world.map->actorAt(cursorPos);
            if (world.map->isSeen(cursorPos) && actorAtPos) {
                terminal_print(0, 24, ("[color=yellow]X[/color] to finish    [color=yellow]SPACE[/color] to examine [color=yellow]" + actorAtPos->getName(true)).c_str());
            } else {
                terminal_print(0, 24, "[color=yellow]X[/color] to finish");
            }
        } else if (uiMode == UIMode::ChooseDirection) {
            terminal_print_ext(0, 20, 80, 1, TK_ALIGN_DEFAULT, uiModeString.c_str());
            terminal_print(0, 24, "Choose direction or [color=yellow]Z[/color] to cancel");
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
                if (uiMode == UIMode::ExamineTile && here == cursorPos) {
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
                if (uiMode == UIMode::ExamineTile && here == cursorPos) {
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
        std::string healthLine = std::to_string(world.player->health) + "/"
                               + std::to_string(world.player->getStat(STAT_HEALTH));
        std::string energyLine = std::to_string(world.player->energy) + "/"
                               + std::to_string(world.player->getStat(STAT_ENERGY));
        terminal_print(72, 2, healthLine.c_str());
        terminal_print(72, 3, energyLine.c_str());

        // stats
        for (int i = 0; i < STAT_EXTRA_COUNT; ++i) {
            int yPos = 5 + i;
            if (i >= STAT_BASE_COUNT) ++yPos;
            std::string text = statName(i) + ": " + std::to_string(world.player->getStat(i));
            terminal_print(61, yPos, text.c_str());
        }

        // (debug) position data
        terminal_printf(61, 14, "Depth: %d", world.map->depth());
        terminal_print(61, 15, ucFirst(world.map->data.name).c_str());
        terminal_printf(61, 17, "Position: %d, %d", world.player->position.x, world.player->position.y);
        terminal_print(61, 18, ("Turn: " + std::to_string(world.currentTurn) + " / " + std::to_string(world.player->speedCounter)).c_str());

        terminal_refresh();

        int key = terminal_read();
        if (key == TK_CLOSE)    break;
        if (uiMode == UIMode::PlayerDead) {
            if (key == TK_ESCAPE)   break;
            if (key == TK_L)        doMessageLog(world);
            if (key == TK_I)        doInventory(world, false);
            if (key == TK_X) {
                uiMode = UIMode::ExamineTile;
                cursorPos = world.player->position;
            }
            if (key == TK_F1) {
                world.player->takeDamage(-99999);
                world.player->spendEnergy(-99999);
                uiMode = UIMode::Normal;
                shownDeathMessage = false;
                world.addMessage("[color=cyan]DEBUG[/color] resurrecting player");
            }
        } else if (uiMode == UIMode::Normal) {
            if (key == TK_ESCAPE)   break;
            Direction theDir = keyToDirection(key);
            if (theDir == Direction::Here) {
                world.player->advanceSpeedCounter();
                world.tick();
            } else if (theDir != Direction::Unknown) {
                tryMovePlayer(world, theDir);
            }

            if (key == TK_L)        doMessageLog(world);
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

            if (key == TK_I)        doInventory(world, false);
            if (key == TK_C)        doCharInfo(world);
            if (key == TK_X) {
                uiMode = UIMode::ExamineTile;
                cursorPos = world.player->position;
            }

            if (key == TK_F1) {
                world.player->takeDamage(-99999);
                world.player->spendEnergy(-99999);
                world.addMessage("[color=cyan]DEBUG[/color] health and energy restored");
            } else if (key == TK_F6) {
                world.disableFOV = !world.disableFOV;
                world.addMessage("[color=cyan]DEBUG[/color] toggled FOV");
            } else if (key == TK_F7) {
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
            } else if (key == TK_F8) {
                uiMode = UIMode::ChooseDirection;
                uiModeString = "[color=cyan]DEBUG[/color] make tunnel";
                uiModeAction = UI_DEBUG_TUNNEL;
            } else if (key == TK_F10) {
                debug_saveMapToPNG(*world.map, true);
                world.addMessage("[color=cyan]DEBUG[/color] wrote dungeon map (including actors) to file");
            } else if (key == TK_F11) {
                debug_saveMapToPNG(*world.map, false);
                world.addMessage("[color=cyan]DEBUG[/color] wrote dungeon map (layout only) to file");
            }
        } else if (uiMode == UIMode::ExamineTile) {
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
                const MapTile *tile = world.map->at(cursorPos);
                if (tile && tile->isSeen && tile->actor) showActorInfo(world, tile->actor);
            }
            Direction theDir = keyToDirection(key);
            if (theDir != Direction::Unknown) {
                cursorPos = cursorPos.shift(theDir);
            }
        } else if (uiMode == UIMode::ChooseDirection) {
            if (key == TK_ESCAPE || key == TK_X || key == TK_MOUSE_RIGHT) {
                uiMode = UIMode::Normal;
            }
            Direction theDir = keyToDirection(key);
            if (theDir != Direction::Unknown) {
                uiMode = UIMode::Normal;
                switch(uiModeAction) {
                    case UI_DEBUG_TUNNEL: {
                        Coord where = world.player->position.shift(theDir);
                        world.map->floorAt(where, TILE_FLOOR);
                        world.addMessage("Carved tunnel.");
                        break; }
                    default:
                        std::cerr << "Unknown UI action " << uiModeAction << '\n';
                }
            }
        } else {
            std::cerr << "unhandled UIMode " << static_cast<int>(uiMode) << " in input \n";
        }
    }

    return;
}