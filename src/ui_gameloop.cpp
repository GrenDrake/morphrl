#include <ctime>
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
void restUntilHealed(World &world);
void tryMovePlayer(World &world, Direction dir);
void tryPlayerTakeItem(World &world);
void tryPlayerChangeFloor(World &world);
void tryPlayerInteractTile(World &world, Direction dir);

void debug_addThing(World &world, int thingType);
void debug_doTeleport(World &world);
void debug_killNeighbours(World &world);


const unsigned MODE_ALL             = 0xFFFFFFFF;
const unsigned MODE_DEAD            = 0x00000001;
const unsigned MODE_NORMAL          = 0x00000002;
const unsigned MODE_EXAMINE_TILE    = 0x00000004;
const unsigned MODE_CHOOSE_DIRECTION = 0x00000008;
const unsigned MODE_CHOOSE_TARGET   = 0x00000010;
const unsigned MODE_PICK_FROM_LIST  = 0x0000020;


const int ACT_FULLQUIT = -2;
const int ACT_NONE = -1;
const int ACT_MENU = 0;
const int ACT_LOG = 1;
const int ACT_CHARINFO = 2;
const int ACT_INVENTORY = 3;
const int ACT_EXAMINETILE = 4;
const int ACT_CHANGEFLOOR = 5;
const int ACT_MOVE = 6;
const int ACT_WAIT = 7;
const int ACT_TAKEITEM = 9;
const int ACT_REST = 10;
const int ACT_INTERACTTILE = 11;
const int ACT_USEABILITY = 12;

const int ACT_DBG_FULLHEAL = 1000;
const int ACT_DBG_TELEPORT = 1001;
const int ACT_DBG_ADDITEM = 1002;
const int ACT_DBG_ADDMUTATION = 1003;
const int ACT_DBG_ADDSTATUS = 1004;
const int ACT_DBG_DISABLEFOV = 1005;
const int ACT_DBG_KILLADJ = 1006;
const int ACT_DBG_TUNNEL = 1007;
const int ACT_DBG_MAPWACTORS = 1008;
const int ACT_DBG_MAP = 1009;
const int ACT_DBG_XP = 1010;


struct KeyBinding {
    int key;
    int action;
    Direction dir;
    unsigned forMode;
};

std::vector<KeyBinding> keyBindings{
    {   TK_CLOSE,  ACT_FULLQUIT,        Direction::Unknown, MODE_ALL },
    
    {   TK_ESCAPE,  ACT_MENU,           Direction::Unknown, MODE_DEAD|MODE_NORMAL },
    {   TK_L,       ACT_LOG,            Direction::Unknown, MODE_DEAD|MODE_NORMAL },
    {   TK_C,       ACT_CHARINFO,       Direction::Unknown, MODE_DEAD|MODE_NORMAL },
    {   TK_I,       ACT_INVENTORY,      Direction::Unknown, MODE_DEAD|MODE_NORMAL },
    {   TK_X,       ACT_EXAMINETILE,    Direction::Unknown, MODE_DEAD|MODE_NORMAL },
    {   TK_COMMA,   ACT_CHANGEFLOOR,    Direction::Unknown, MODE_NORMAL },
    {   TK_PERIOD,  ACT_CHANGEFLOOR,    Direction::Unknown, MODE_NORMAL },
    {   TK_LEFT,    ACT_MOVE,           Direction::West,    MODE_NORMAL },
    {   TK_RIGHT,   ACT_MOVE,           Direction::East,    MODE_NORMAL },
    {   TK_UP,      ACT_MOVE,           Direction::North,   MODE_NORMAL },
    {   TK_DOWN,    ACT_MOVE,           Direction::South,   MODE_NORMAL },
    {   TK_SPACE,   ACT_WAIT,           Direction::Unknown, MODE_NORMAL },
    {   TK_T,       ACT_TAKEITEM,       Direction::Unknown, MODE_NORMAL },
    {   TK_G,       ACT_TAKEITEM,       Direction::Unknown, MODE_NORMAL },
    {   TK_R,       ACT_REST,           Direction::Unknown, MODE_NORMAL },
    {   TK_O,       ACT_INTERACTTILE,   Direction::Unknown, MODE_NORMAL },
    {   TK_A,       ACT_USEABILITY,     Direction::Unknown, MODE_NORMAL },

#ifdef DEBUG
    {   TK_F1,      ACT_DBG_FULLHEAL,   Direction::Unknown, MODE_DEAD|MODE_NORMAL },
    {   TK_F2,      ACT_DBG_TELEPORT,   Direction::Unknown, MODE_NORMAL },
    {   TK_F3,      ACT_DBG_ADDITEM,    Direction::Unknown, MODE_NORMAL },
    {   TK_F4,      ACT_DBG_ADDMUTATION,Direction::Unknown, MODE_NORMAL },
    {   TK_F5,      ACT_DBG_ADDSTATUS,  Direction::Unknown, MODE_NORMAL },
    {   TK_F6,      ACT_DBG_DISABLEFOV, Direction::Unknown, MODE_DEAD|MODE_NORMAL },
    {   TK_F7,      ACT_DBG_KILLADJ,    Direction::Unknown, MODE_NORMAL },
    {   TK_F8,      ACT_DBG_TUNNEL,     Direction::Unknown, MODE_NORMAL },
    {   TK_F10,     ACT_DBG_MAPWACTORS, Direction::Unknown, MODE_DEAD|MODE_NORMAL },
    {   TK_F11,     ACT_DBG_MAP,        Direction::Unknown, MODE_DEAD|MODE_NORMAL },
    {   TK_F12,     ACT_DBG_XP,         Direction::Unknown, MODE_NORMAL },
#endif
};

const KeyBinding NO_KEY{ 0, ACT_NONE };
const KeyBinding& getBindingForKey(int keyPressed, unsigned currentMode) {
    for (const KeyBinding &binding : keyBindings) {
        if ((binding.forMode & currentMode) != currentMode) continue;
        if (binding.key == keyPressed) return binding;
    }
    return NO_KEY;
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
        if (!tile->everSeen || tile->floor == TILE_NOTHING) {
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

void glyphAndColourForCoord(const World &world, const Coord &where, int &glyph, color_t &bkcolor, color_t &color) {
    // const color_t hotZone = color_from_argb(255, 98, 32, 32);
    // const color_t hotZoneDark = color_from_argb(255, 49, 16, 16);
    // const color_t coldZone = color_from_argb(255, 0, 64, 64);
    // const color_t coldZoneDark = color_from_argb(255, 0, 32, 32);
    if (!world.map->isValidPosition(where)) {
        color = color_from_argb(255, 192, 192, 192);
        bkcolor = color_from_argb(255, 0, 0, 0);
        glyph = ' ';
        return;
    }

    const MapTile *tile = world.map->at(where);
    const TileData &td = getTileData(tile->floor);
    const Actor *actor = tile->actor;
    const Item *item = tile->items.empty() ? nullptr : tile->items.front();

    bkcolor = color_from_argb(255, 0, 0, 0);

    // if we've never seen the space, don't show it
    if (!world.disableFOV && !tile->everSeen) {
        color = bkcolor;
        glyph = ' ';
        return;
    }

    // if we can currently see the space, show its content
    if (world.disableFOV || tile->isSeen) {
        if (actor) {
            color = color_from_argb(255, actor->data.r, actor->data.g, actor->data.b);
            glyph = actor->data.glyph;
        } else if (item) {
            color = color_from_argb(255, item->data.r, item->data.g, item->data.b);
            glyph = item->data.glyph;
        } else {
            color = color_from_argb(255, td.r, td.g, td.b);
            glyph = td.glyph;
        }
    // otherwise just show the terrain
    } else {
        color = color_from_argb(255, td.r / 2, td.g / 2, td.b / 2);
        glyph = td.glyph;
    }
    return;
}


void drawStatBar(int x, int y, int amount, int maximum, color_t filledColour) {
    const color_t depletedColour = color_from_argb(255, 127, 127, 127);
    terminal_bkcolor(filledColour);
    int fillPercent = 0;
    if (amount < 1) fillPercent = 0;
    else {
        fillPercent = percentOf(amount, maximum) / 10;
        if (fillPercent < 1) fillPercent = 1;
    }

    for (int i = 0; i < 10; ++i) {
        if (i >= fillPercent) terminal_bkcolor(depletedColour);
        terminal_put(x+i, y, ' ');
    }
}

struct ListItem {
    std::string name;
    unsigned value;
};

const int UI_DEBUG_TUNNEL = 10000;
const int UI_USE_ABILITY  = 10001;
const int UI_INTERACT_TILE  = 10002;
GameReturn gameloop(World &world) {
    const color_t black = color_from_argb(255, 0, 0, 0);
    const color_t cursorColour = color_from_argb(255, 127, 127, 127);
    const color_t targetLineColour = color_from_argb(255, 63, 63, 63);
    const color_t alertTextColour = color_from_argb(255, 192, 63, 63);
    const color_t textColour = color_from_argb(255, 192, 192, 192);
    const color_t healthColour = color_from_argb(255, 255, 127, 127);
    const color_t energyColour = color_from_argb(255, 127, 127, 255);

    const unsigned animDelay = configData.getIntValue("animation_delay", 300);
    std::string uiModeString;
    int uiModeAction = 0;
    unsigned uiMode = MODE_NORMAL;
    Coord cursorPos(-1, -1);
    bool shownDeathMessage = false;
    std::vector<Coord> targetArea;
    unsigned uiModeParam = 0;
    int targetAreaType = AR_NONE;
    int targetAreaRange = 0;
    std::vector<ListItem> uiListOfThings;
    clock_t timer = 0;
    while (1) {
        if (world.gameState == GameState::Victory) {
            showDocument("ending.txt");
            return GameReturn::Normal;
        }

        world.player->verify();
        if (uiMode == MODE_NORMAL && world.player->isDead()) {
            uiMode = MODE_DEAD;
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
        if (uiMode == MODE_NORMAL || uiMode == MODE_DEAD) {
            for (auto iter = world.messages.rbegin(); iter != world.messages.rend(); ++iter) {
                dimensions_t dims = terminal_measure_ext(77, 5, iter->text.c_str());
                if (dims.height > 1) yPos -= dims.height - 1;
                terminal_put(0, yPos, '*');
                terminal_print_ext(2, yPos, 77, 5, TK_ALIGN_DEFAULT, iter->text.c_str());
                --yPos;
                if (yPos < 20) break;
            }
        } else if (uiMode == MODE_EXAMINE_TILE) {
            const std::string desc = previewMapSpace(world, cursorPos);
            terminal_print_ext(0, 20, 80, 5, TK_ALIGN_DEFAULT, desc.c_str());
            const Actor *actorAtPos = world.map->actorAt(cursorPos);
            if (world.map->isSeen(cursorPos) && actorAtPos) {
                terminal_print(0, 24, ("[color=yellow]X[/color] to finish    [color=yellow]SPACE[/color] to examine [color=yellow]" + actorAtPos->getName(true)).c_str());
            } else {
                terminal_print(0, 24, "[color=yellow]X[/color] to finish");
            }
        } else if (uiMode == MODE_CHOOSE_TARGET) {
            const std::string desc = previewMapSpace(world, cursorPos);
            terminal_print_ext(0, 21, 80, 5, TK_ALIGN_DEFAULT, desc.c_str());
            terminal_print_ext(0, 20, 80, 1, TK_ALIGN_DEFAULT, uiModeString.c_str());
            terminal_print(0, 24, "Choose target space or [color=yellow]Z[/color] to cancel");
        } else if (uiMode == MODE_CHOOSE_DIRECTION) {
            terminal_print_ext(0, 20, 80, 1, TK_ALIGN_DEFAULT, uiModeString.c_str());
            terminal_print(0, 24, "Choose direction or [color=yellow]Z[/color] to cancel");
        } else if (uiMode == MODE_PICK_FROM_LIST) {
            terminal_print_ext(0, 20, 80, 1, TK_ALIGN_DEFAULT, uiModeString.c_str());
            int xPos = 0, yPos = 21, counter = 1;
            for (unsigned i = 0; i < uiListOfThings.size(); ++i) {
                terminal_printf(xPos, yPos, "%d) %s", counter, ucFirst(uiListOfThings[i].name).c_str());
                ++yPos;
                ++counter;
            }
        } else {
            logMessage(LOG_ERROR, "Unsupported UIMode " + std::to_string(static_cast<int>(uiMode)) + " in display");
        }
        terminal_clear_area(0, 0, 80, 20);

        // draw interface frame
        for (int x = 0; x < 80; ++x) terminal_put(x, 19, 0x2500);

        // draw map
        for (int y = 0; y < 19; ++y) {
            for (int x = 0; x < 80; ++x) {
                Coord here(Coord(offsetX + x, offsetY + y));
                color_t fgColor, bgColor;
                int glyph;
                glyphAndColourForCoord(world, here, glyph, bgColor, fgColor);

                terminal_color(fgColor);
                terminal_bkcolor(bgColor);
                if (uiMode == MODE_EXAMINE_TILE && here == cursorPos) {
                    terminal_bkcolor(cursorColour);
                } else if (uiMode == MODE_CHOOSE_TARGET) {
                    if (here == cursorPos) {
                        terminal_bkcolor(cursorColour);
                    } else if (vectorContains(targetArea, here)) terminal_bkcolor(targetLineColour);
                } else if (world.map->inOverlay(here)) {
                    terminal_color(color_from_argb(255, world.map->overlayR, world.map->overlayG, world.map->overlayB));
                    terminal_put(x, y, world.map->overlayGlyph);
                    continue;
                }

                terminal_put(x, y, glyph);
            }
        }

        // Health, energy, and level name info
        std::string healthLine = std::to_string(world.player->health) + "/"
                               + std::to_string(world.player->getStat(STAT_HEALTH));
        std::string energyLine = std::to_string(world.player->energy) + "/"
                               + std::to_string(world.player->getStat(STAT_ENERGY));
        drawStatBar( 4, 19, world.player->health, world.player->getStat(STAT_HEALTH), healthColour);
        drawStatBar(23, 19, world.player->energy, world.player->getStat(STAT_ENERGY), energyColour);
        terminal_bkcolor(black);
        if (world.player->isOverBurdened()) {
            terminal_color(alertTextColour);
            terminal_print(41, 19, "* OVER-BURDENED *");
        }
        terminal_color(textColour);
        terminal_print(15, 19, healthLine.c_str());
        terminal_print(34, 19, energyLine.c_str());
        terminal_print(79 - world.map->data.name.size(), 19, ucFirst(world.map->data.name).c_str());

#ifdef DEBUG
        // (debug) position data
        terminal_printf(61, 0, "(%d, %d) Lv%d", world.player->position.x, world.player->position.y, world.map->depth());
        terminal_printf(61, 1, "Turn: %u / %u", world.currentTurn, world.player->speedCounter);
        clock_t newTimer = clock();
        unsigned timeTaken = (newTimer - timer) * 1000 / CLOCKS_PER_SEC;
        timer = newTimer;
        terminal_printf(61, 2, "Tick time: %u", timeTaken);
        terminal_printf(61, 3, "Since Combat: %u", world.player->turnsSinceCombatAction);
#endif

        terminal_refresh();

        if (!world.map->overlayTiles.empty()) {
            terminal_delay(animDelay);
            world.map->overlayTiles.clear();
            continue;
        }
        if (!world.player->isDead() && !world.map->getNextActor()->isPlayer) {
            world.tick();
            continue;
        }

        // ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
        // INPUT HANDLING
        int key = terminal_read();
#ifdef DEBUG
        timer = clock();
#endif

        if (key == TK_MOUSE_RIGHT && (uiMode == MODE_NORMAL || uiMode == MODE_DEAD)) {
            int mx = terminal_state(TK_MOUSE_X);
            int my = terminal_state(TK_MOUSE_Y);
            if (my < 19) {
                Coord where(mx + offsetX, my + offsetY);
                world.addMessage(previewMapSpace(world, where));
            }
            continue;
        }

        const KeyBinding &action = getBindingForKey(key, uiMode);

        if (action.action == ACT_FULLQUIT) return GameReturn::FullQuit;

        // ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
        // UIMode::Normal  UIMode::PlayerDead
        if (uiMode == MODE_NORMAL || uiMode == MODE_DEAD) {
            if (action.action == ACT_NONE) continue;
            if (action.action == ACT_MENU) break;
            if (action.action == ACT_LOG) doMessageLog(world);
            if (action.action == ACT_CHARINFO) doCharInfo(world);
            if (action.action == ACT_INVENTORY) doInventory(world, false);
            if (action.action == ACT_TAKEITEM) tryPlayerTakeItem(world);
            if (action.action == ACT_REST) restUntilHealed(world);

            if (action.action == ACT_EXAMINETILE) {
                    uiMode = MODE_EXAMINE_TILE;
                    cursorPos = world.player->position;
            }
            if (action.action == ACT_INTERACTTILE) {
                uiMode = MODE_CHOOSE_DIRECTION;
                uiModeString = "Interact where?";
                uiModeAction = UI_INTERACT_TILE;
            }

            if (action.action == ACT_WAIT) {
                world.player->advanceSpeedCounter();
                world.tick();
            }
            if (action.action == ACT_MOVE) {
                tryMovePlayer(world, action.dir);
            }
            if (action.action == ACT_CHANGEFLOOR) tryPlayerChangeFloor(world);

            if (action.action == ACT_USEABILITY) {
                std::vector<unsigned> abilityList = world.player->getAbilityList();
                if (abilityList.empty()) {
                    world.addMessage("You have no special abilities.");
                } else {
                    uiListOfThings.clear();
                    for (unsigned i : abilityList) {
                        const AbilityData &data = getAbilityData(i);
                        if (data.areaType == AR_PASSIVE) continue;
                        if (data.energyCost <= world.player->energy) {
                            std::string optionName = data.name + " (" + std::to_string(data.energyCost) + ")";
                            uiListOfThings.push_back(ListItem{optionName, data.ident});
                        }
                    }
                    if (uiListOfThings.empty()) {
                        world.addMessage("You have no abilities you can use right now.");
                    } else {
                        uiListOfThings.push_back(ListItem{"Cancel", BAD_VALUE});
                        uiMode = MODE_PICK_FROM_LIST;
                        uiModeAction = UI_USE_ABILITY;
                        uiModeString = "Use which ability?";
                    }
                }
            }

#ifdef DEBUG
            if (action.action == ACT_DBG_FULLHEAL) {
                if (world.player->isDead()) world.addMessage("[color=cyan]DEBUG[/color] resurrecting player");
                else world.addMessage("[color=cyan]DEBUG[/color] health and energy restored");
                world.player->takeDamage(-99999, nullptr);
                world.player->spendEnergy(-99999);
                uiMode = MODE_NORMAL;
                shownDeathMessage = false;
            } else if (action.action == ACT_DBG_TELEPORT)    debug_doTeleport(world);
            else if (action.action == ACT_DBG_ADDITEM)      debug_addThing(world, TT_ITEM);
            else if (action.action == ACT_DBG_ADDMUTATION)      debug_addThing(world, TT_MUTATION);
            else if (action.action == ACT_DBG_ADDSTATUS)      debug_addThing(world, TT_STATUS_EFFECT);
            else if (action.action == ACT_DBG_DISABLEFOV) {
                world.disableFOV = !world.disableFOV;
                world.addMessage("[color=cyan]DEBUG[/color] toggled FOV");
            } else if (action.action == ACT_DBG_KILLADJ) debug_killNeighbours(world);
            else if (action.action == ACT_DBG_TUNNEL) {
                uiMode = MODE_CHOOSE_DIRECTION;
                uiModeString = "[color=cyan]DEBUG[/color] make tunnel";
                uiModeAction = UI_DEBUG_TUNNEL;
            } else if (action.action == ACT_DBG_MAPWACTORS) {
                debug_saveMapToPNG(*world.map, true);
                world.addMessage("[color=cyan]DEBUG[/color] wrote dungeon map (including actors) to file");
            } else if (action.action == ACT_DBG_MAP) {
                debug_saveMapToPNG(*world.map, false);
                world.addMessage("[color=cyan]DEBUG[/color] wrote dungeon map (layout only) to file");
            } else if (action.action == ACT_DBG_XP) {
                world.player->giveXP(50);
                world.addMessage("[color=cyan]DEBUG[/color] granted XP points");
            }

#endif

        // ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
        // UIMode::ExamineTile
        } else if (uiMode == MODE_EXAMINE_TILE) {
            if (key == TK_MOUSE_LEFT) {
                int mx = terminal_state(TK_MOUSE_X);
                int my = terminal_state(TK_MOUSE_Y);
                if (my < 19) {
                    cursorPos.x = mx + offsetX;
                    cursorPos.y = my + offsetY;
                }
            }
            if (key == TK_ESCAPE || key == TK_X || key == TK_MOUSE_RIGHT) {
                uiMode = MODE_NORMAL;
            }
            if (key == TK_ENTER || key == TK_SPACE || key == TK_KP_ENTER) {
                uiMode = MODE_NORMAL;
                const MapTile *tile = world.map->at(cursorPos);
                if (tile && tile->isSeen && tile->actor) showActorInfo(world, tile->actor);
            }
            Direction theDir = keyToDirection(key);
            if (theDir != Direction::Unknown) {
                cursorPos = cursorPos.shift(theDir);
            }

        // ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
        // UIMode::PickFromList
        } else if (uiMode == MODE_PICK_FROM_LIST) {
            if (key == TK_ESCAPE || key == TK_X || key == TK_MOUSE_RIGHT) {
                uiMode = MODE_NORMAL;
            }
            int choice = -1;
            if (key == TK_1 && uiListOfThings.size() >= 1) choice = 0;
            if (key == TK_2 && uiListOfThings.size() >= 2) choice = 1;
            if (key == TK_3 && uiListOfThings.size() >= 3) choice = 2;
            if (key == TK_4 && uiListOfThings.size() >= 4) choice = 3;
            if (key == TK_5 && uiListOfThings.size() >= 5) choice = 4;
            if (key == TK_6 && uiListOfThings.size() >= 6) choice = 5;
            if (key == TK_7 && uiListOfThings.size() >= 7) choice = 6;
            if (key == TK_8 && uiListOfThings.size() >= 8) choice = 7;
            if (key == TK_9 && uiListOfThings.size() >= 9) choice = 8;
            if (key == TK_0 && uiListOfThings.size() >= 10) choice = 9;
            if (uiModeAction != UI_USE_ABILITY) {
                world.addMessage("Unhandled action in MODE_PICK_FROM_LIST");
                continue;
            }
            if (choice >= 0) {
                uiMode = MODE_NORMAL;
                unsigned ident = uiListOfThings[choice].value;
                if (ident == BAD_VALUE) continue;
                const AbilityData &data = getAbilityData(ident);
                if (data.ident == BAD_VALUE) {
                    world.addMessage("Tried to use invalid ability");
                } else {
                    if (data.areaType == AR_NONE || data.areaType == AR_BURST) {
                        targetArea = world.map->getEffectArea(world.player->position, world.player->position, data.areaType, data.maxRange, false, false);
                        world.map->activateAbility(world, data.ident, world.player->position, targetArea);
                        world.player->advanceSpeedCounter(data.speedMult);
                        world.tick();
                    } else if (data.areaType == AR_CONE) {
                        targetAreaRange = data.maxRange;
                        targetAreaType = data.areaType;
                        uiModeParam = ident;
                        uiMode = MODE_CHOOSE_DIRECTION;
                        uiModeString = "Choose direction for " + data.name + ".";
                        targetArea = world.map->getEffectArea(world.player->position, cursorPos, targetAreaType, targetAreaRange, false, false);
                    } else {
                        cursorPos = world.player->position;
                        targetAreaRange = data.maxRange;
                        targetAreaType = data.areaType;
                        uiModeParam = ident;
                        uiMode = MODE_CHOOSE_TARGET;
                        uiModeString = "Choose target for " + data.name + ".";
                        targetArea = world.map->getEffectArea(world.player->position, cursorPos, targetAreaType, targetAreaRange, false, false);
                    }
                }
            }

        // ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
        // UIMode::ChooseTarget
        } else if (uiMode == MODE_CHOOSE_TARGET) {
            if (key == TK_MOUSE_LEFT) {
                int mx = terminal_state(TK_MOUSE_X);
                int my = terminal_state(TK_MOUSE_Y);
                if (my < 19) {
                    cursorPos.x = mx + offsetX;
                    cursorPos.y = my + offsetY;
                    targetArea = world.map->getEffectArea(world.player->position, cursorPos, targetAreaType, targetAreaRange, false, false);
                }
            }
            if (key == TK_ESCAPE || key == TK_X || key == TK_MOUSE_RIGHT) {
                uiMode = MODE_NORMAL;
            }
            if (key == TK_ENTER || key == TK_SPACE || key == TK_KP_ENTER) {
                uiMode = MODE_NORMAL;
                // DO THE THING
                if (uiModeAction == UI_USE_ABILITY) {
                    const AbilityData &abilityData = getAbilityData(uiModeParam);
                    world.map->activateAbility(world, uiModeParam, cursorPos, targetArea);
                    world.player->advanceSpeedCounter(abilityData.speedMult);
                    world.tick();
                } else {
                    world.addMessage("ERROR unhandled ui action in MODE_CHOOSE_TARGET");
                }
            }
            Direction theDir = keyToDirection(key);
            if (theDir != Direction::Unknown) {
                cursorPos = cursorPos.shift(theDir);
                targetArea = world.map->getEffectArea(world.player->position, cursorPos, targetAreaType, targetAreaRange, false, false);
            }

        // ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
        // UIMode::ChooseDirection
        } else if (uiMode == MODE_CHOOSE_DIRECTION) {
            if (key == TK_ESCAPE || key == TK_X || key == TK_MOUSE_RIGHT) {
                uiMode = MODE_NORMAL;
            }
            Direction theDir = keyToDirection(key);
            if (theDir != Direction::Unknown) {
                uiMode = MODE_NORMAL;
                switch(uiModeAction) {
                    case UI_DEBUG_TUNNEL: {
                        Coord where = world.player->position.shift(theDir);
                        world.map->floorAt(where, TILE_FLOOR);
                        world.addMessage("Carved tunnel.");
                        break; }
                    case UI_USE_ABILITY: {
                        const AbilityData &abilityData = getAbilityData(uiModeParam);
                        targetArea = world.map->getEffectArea(world.player->position, world.player->position.shift(theDir), targetAreaType, targetAreaRange, false, false);
                        world.map->activateAbility(world, uiModeParam, cursorPos, targetArea);
                        world.player->advanceSpeedCounter(abilityData.speedMult);
                        world.tick();
                        break; }
                    case UI_INTERACT_TILE: {
                        tryPlayerInteractTile(world, theDir);
                        break;
                    }
                    default:
                        logMessage(LOG_ERROR, "Unknown UI action " + std::to_string(uiModeAction));
                }
            }
        } else {
            logMessage(LOG_ERROR, "unhandled UIMode " + std::to_string(static_cast<int>(uiMode)) + " in input");
        }
    }

    return GameReturn::Normal;
}