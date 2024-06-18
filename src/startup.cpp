#include <cstdint>
#include <ctime>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "physfs.h"
#include "BearLibTerminal.h"
#include "morph.h"


GameReturn gameloop(World &world);
GameReturn keyBindingsMenu();
void doDebugCodex();

RNG globalRNG;

World* createGame(uint64_t gameSeed, unsigned iteration) {
    if (iteration > 50) {
        logMessage(LOG_ERROR, " world generation experienced catastraphic failure");
        return nullptr;
    }
    World *world = new World;
    if (gameSeed == 0)  world->gameSeed = globalRNG.next32();
    else                world->gameSeed = gameSeed;
    logMessage(LOG_INFO, "NEW GAME with seed: " + std::to_string(world->gameSeed));
    world->player = Actor::create(getActorData(0));
    world->player->isPlayer = true;
    world->player->reset();
    if (!world->movePlayerToDepth(getDungeonEntranceIdent(), DE_ENTRANCE)) {
        logMessage(LOG_INFO, "world generation failed");
        uint64_t newSeed = world->gameSeed + 1;
        delete world;
        return createGame(newSeed, iteration + 1);
    }
    world->addMessage("Welcome to [color=yellow]MorphRL[/color]!");
    return world;
}

struct UIRect {
    int x, y, w, h, ident;
};

int keyToInt(int key) {
    switch(key) {
        case TK_0: return 0;
        case TK_1: return 1;
        case TK_2: return 2;
        case TK_3: return 3;
        case TK_4: return 4;
        case TK_5: return 5;
        case TK_6: return 6;
        case TK_7: return 7;
        case TK_8: return 8;
        case TK_9: return 9;
        default:   return -1;
    }
}

int main(int argc, char *argv[]) {
    PHYSFS_init(argv[0]);
    const char *writeDir = PHYSFS_getPrefDir("grendrake", "morphrl");
    if (!writeDir) {
        std::string errorMessage = "failed to find suitable location for writing files: ";
        errorMessage += PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode());
        errorMessage += ".\n";
        logMessage(LOG_ERROR, errorMessage);
        return 1;
    }
    std::cerr << "Write directory: " << writeDir << '\n';
    if (PHYSFS_setWriteDir(writeDir) == 0) {
        std::string errorMessage = "failed to set write directory: " +
        errorMessage += PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode());
        errorMessage += ".\n";
        logMessage(LOG_ERROR, errorMessage);
        return 1;
    }
    PHYSFS_mount(writeDir, "/saves", 1);
    PHYSFS_mount(PHYSFS_getBaseDir(), "/root", 1);
    loadConfigData("game.cfg");
    PHYSFS_mount("resources", "/", 1);
    PHYSFS_mount("gamedata.dat", "/", 1);
    if (!loadAllData()) return 1;

    int fontSize = configData.getIntValue("fontsize", 24);


    terminal_open();
    if (configData.getBoolValue("fullscreen", false)) terminal_set("window.fullscreen = true");
    terminal_set("window.title='MorphRL';");
    terminal_set("input.filter = [keyboard, mouse];");
    terminal_setf("font: DejaVuSansMono.ttf, size=%d;", fontSize);
    terminal_setf("italic font: DejaVuSansMono-Oblique.ttf, size=%d;", fontSize);

#ifdef DEBUG
    const int menuItemCount = 8;
#else
    const int menuItemCount = 7;
#endif
    std::vector<UIRect> mouseRegions;
    World *world = nullptr;
    const std::string versionString = "Version: Alpha-2";
    const int versionX = (79 - versionString.size()) / 2;
    Image *logo = loadImage("logo.png");
    bool done = false;
    int selection = 0;
    color_t fgColor = color_from_argb(255, 196, 196, 196);
    color_t fgColorDark = color_from_argb(255, 98, 98, 98);
    color_t bgColor = color_from_argb(255, 0, 0, 0);
    while (!done) {
        mouseRegions.clear();
        terminal_color(fgColor);
        terminal_bkcolor(bgColor);
        terminal_clear();
        terminal_print(25, 11, "[font=italic]Delving the Mutagenic Dungeon");
        terminal_print(8, 16, "Start new game");
        mouseRegions.push_back(UIRect{8, 16, 20, 1, 0});
        terminal_print(8, 17, "Start game from seed");
        mouseRegions.push_back(UIRect{8, 17, 20, 1, 1});
        terminal_print(8, 18, "Resume current game");
        mouseRegions.push_back(UIRect{8, 18, 20, 1, 2});
        terminal_print(8, 19, "Story");
        mouseRegions.push_back(UIRect{8, 19, 20, 1, 3});
        terminal_print(8, 20, "Instructions");
        mouseRegions.push_back(UIRect{8, 20, 20, 1, 4});
        terminal_print(8, 21, "Key Bindings");
        mouseRegions.push_back(UIRect{8, 21, 20, 1, 5});
        terminal_print(8, 22, "Credits");
        mouseRegions.push_back(UIRect{8, 22, 20, 1, 6});
        terminal_print(8, 23, "Quit");
        mouseRegions.push_back(UIRect{8, 23, 20, 1, 7});
#ifdef DEBUG
        terminal_print(8, 24, "Data Codex ([color=yellow]DEBUG[/color])");
        mouseRegions.push_back(UIRect{8, 24, 20, 1, 8});
#endif
        terminal_print(5, 16+selection, "->");
        terminal_color(fgColorDark);
        terminal_print(versionX, 12, versionString.c_str());
#ifdef DEBUG
        terminal_color(color_from_argb(255, 255, 0, 0));
        terminal_print(33, 13, "DEBUG VERSION");
#endif
        drawImage(0, 0, logo);
        terminal_refresh();

        int key = terminal_read();
        if (key == TK_Q || key == TK_CLOSE) {
            break;
        }
        if (key == TK_HOME) selection = 0;
        if ((key == TK_UP || key == TK_KP_8) && selection > 0) --selection;
        if (key == TK_END) selection = menuItemCount;
        if ((key == TK_DOWN || key == TK_KP_2) && selection < menuItemCount) ++selection;

        if (key == TK_MOUSE_LEFT) {
            int mx = terminal_state(TK_MOUSE_X);
            int my = terminal_state(TK_MOUSE_Y);
            for (const UIRect &rect : mouseRegions) {
                if (mx < rect.x || mx >= rect.x + rect.w) continue;
                if (my < rect.y || my >= rect.y + rect.h) continue;
                selection = rect.ident;
                key = TK_SPACE;
                break;
            }
        }

        if (key == TK_SPACE || key == TK_ENTER || key == TK_KP_ENTER) {
            switch(selection) {
                case 0:
                case 1: {
                    unsigned newGameSeed = 0;
                    if (selection == 1) {
                        std::string result;
                        if (!ui_getString("Seed", "Input seed for new game", result) || result.empty()) break;
                        if (!strToInt(result, newGameSeed) || newGameSeed == 0) {
                            ui_alertBox("Error", "Invalid game seed. Game seeds must be non-zero integer values.");
                            break;
                        }
                    }
                    // start new game
                    if (world) {
                        delete world;
                    }
                    world = createGame(newGameSeed, 0);
                    if (!world) {
                        ui_alertBox("Error", "Could not create game world.");
                    } else {
                        GameReturn ret = gameloop(*world);
                        if (ret == GameReturn::FullQuit || world->gameState == GameState::Victory) {
                            delete world;
                            world = nullptr;
                            if (ret == GameReturn::FullQuit) done = true;
                        }
                        selection = 2;
                    }
                    break; }
                case 2:
                    if (world) {
                        GameReturn ret = gameloop(*world);
                        if (ret == GameReturn::FullQuit || world->gameState == GameState::Victory) {
                            delete world;
                            world = nullptr;
                            if (ret == GameReturn::FullQuit) done = true;
                        }
                    } else {
                        ui_alertBox("Error", "No game in progress.");
                    }
                    break;
                case 3:
                    if (showDocument("story.txt") == GameReturn::FullQuit) done = true;
                    break;
                case 4:
                    if (showDocument("instructions.txt") == GameReturn::FullQuit) done = true;
                    break;
                case 5:
                    if (keyBindingsMenu() == GameReturn::FullQuit) done = true;
                    break;
                case 6:
                    if (showDocument("credits.txt") == GameReturn::FullQuit) done = true;
                    break;
                case 7: // quit
                    done = true;
                    break;

                case 8:
                    doDebugCodex();
                    break;
            }
        }
    }

    if (world) delete world;
    delete logo;
    terminal_close();

    PHYSFS_deinit();
    return 0;
}


std::map<int, std::string> actionNames{
    { ACT_FULLQUIT,     "Quit to OS" },
    { ACT_NONE,         "None" },
    { ACT_MENU,         "Return to Menu" },
    { ACT_LOG,          "Message Log" },
    { ACT_CHARINFO,     "Character Info" },
    { ACT_INVENTORY,    "View Inventory" },
    { ACT_EXAMINETILE,  "Examine Map" },
    { ACT_CHANGEFLOOR,  "Ascend or Descend" },
    { ACT_MOVE,         "Move" },
    { ACT_WAIT,         "Wait" },
    { ACT_TAKEITEM,     "Take Item" },
    { ACT_REST,         "Rest Until Healed" },
    { ACT_INTERACTTILE, "Interact" },
    { ACT_USEABILITY,   "Use Ability" },

    { ACT_DBG_FULLHEAL, "Full Heal (DEBUG)" },
    { ACT_DBG_TELEPORT, "Teleport (DEBUG)" },
    { ACT_DBG_ADDITEM, "Add Item (DEBUG)" },
    { ACT_DBG_ADDMUTATION, "Add Mutation (DEBUG)" },
    { ACT_DBG_ADDSTATUS, "Add Status Effect (DEBUG)" },
    { ACT_DBG_DISABLEFOV, "Disable FOV (DEBUG)" },
    { ACT_DBG_KILLADJ, "Kill Adjacent Actors (DEBUG)" },
    { ACT_DBG_TUNNEL, "Make Tunnel (DEBUG)" },
    { ACT_DBG_MAPWACTORS, "Write Map and Actors to File  (DEBUG)" },
    { ACT_DBG_MAP, "Write Map to file (DEBUG)" },
    { ACT_DBG_XP, "Give XP (DEBUG)" },
};
const std::string STR_UNKNOWN_ACTION = "Unknown Action";
const std::string& getNameForAction(int action) {
    auto iter = actionNames.find(action);
    if (iter == actionNames.end()) return STR_UNKNOWN_ACTION;
    return iter->second;
}

std::map<int, std::string> keyNames{
    { TK_A,         "A" },
    { TK_B,         "B" },
    { TK_C,         "C" },
    { TK_D,         "D" },
    { TK_E,         "E" },
    { TK_F,         "F" },
    { TK_G,         "G" },
    { TK_H,         "H" },
    { TK_I,         "I" },
    { TK_J,         "J" },
    { TK_K,         "K" },
    { TK_L,         "L" },
    { TK_M,         "M" },
    { TK_N,         "N" },
    { TK_O,         "O" },
    { TK_P,         "P" },
    { TK_Q,         "Q" },
    { TK_R,         "R" },
    { TK_S,         "S" },
    { TK_T,         "T" },
    { TK_U,         "U" },
    { TK_V,         "V" },
    { TK_W,         "W" },
    { TK_X,         "X" },
    { TK_Y,         "Y" },
    { TK_Z,         "Z" },
    { TK_PERIOD,    "." },
    { TK_COMMA,     "," },
    { TK_SPACE,     "<SPACE>" },
    { TK_LEFT,      "<LEFT>" },
    { TK_RIGHT,     "<RIGHT>" },
    { TK_UP,        "<UP>" },
    { TK_DOWN,      "<DOWN>" },
    { TK_ESCAPE,    "<ESCAPE>" },
    { TK_CLOSE,     "<CLOSE>" },
    { TK_F1,        "<F1>" },
    { TK_F2,        "<F2>" },
    { TK_F3,        "<F3>" },
    { TK_F4,        "<F4>" },
    { TK_F5,        "<F5>" },
    { TK_F6,        "<F6>" },
    { TK_F7,        "<F7>" },
    { TK_F8,        "<F8>" },
    { TK_F9,        "<F9>" },
    { TK_F10,       "<F10>" },
    { TK_F11,       "<F11>" },
    { TK_F12,       "<F12>" },
};
const std::string& getNameForKey(int key) {
    static std::string unknownKeyStr;
    auto iter = keyNames.find(key);
    if (iter == keyNames.end()) {
        unknownKeyStr = "<" + std::to_string(key) + ">";
        return unknownKeyStr;
    }
    return iter->second;
}


GameReturn keyBindingsMenu() {
    std::vector<UIRect> mouseRegions;
    const std::string versionString = "Version: Alpha-2";
    bool done = false;
    color_t fgColor = color_from_argb(255, 196, 196, 196);
    color_t fgColorDark = color_from_argb(255, 98, 98, 98);
    color_t bgColor = color_from_argb(255, 0, 0, 0);
    int top = 0, selection = 0;
    const int maxToShow = 20;
    int menuItemCount = keyBindings.size() - 1;
    while (!done) {
        mouseRegions.clear();
        terminal_color(fgColor);
        terminal_bkcolor(bgColor);
        terminal_clear();
        terminal_print(0, 0, "[font=italic]MorphRL[/font]: Delving the Mutagenic Dungeon");
        terminal_print(0, 24, "ESC to return    UP / DOWN to Scroll List");

        for (unsigned i = 0; i < keyBindings.size(); ++i) {
            top = selection - maxToShow / 2;
            if (top < 0) top = 0;
            int yLine = i + 2 - top;
            if (yLine < 2) continue;
            if (yLine > maxToShow + 2) break;
            const KeyBinding &binding = keyBindings[i];

            if (i == static_cast<unsigned>(selection)) {
                terminal_color(bgColor);
                terminal_bkcolor(fgColor);
                terminal_clear_area(0, yLine, 80, 1);
            }
            terminal_print(1, yLine, getNameForKey(binding.key).c_str());
            if (binding.dir == Direction::Unknown) {
                terminal_print(15, yLine, getNameForAction(binding.action).c_str());
            } else {
                std::stringstream text;
                text << getNameForAction(binding.action) << " " << binding.dir;
                terminal_print(15, yLine, text.str().c_str());
            }
            if (i == static_cast<unsigned>(selection)) {
                terminal_color(fgColor);
                terminal_bkcolor(bgColor);
            }
        }

        terminal_refresh();

        int key = terminal_read();
        if (key == TK_HOME) selection = 0;
        if ((key == TK_UP || key == TK_KP_8) && selection > 0) --selection;
        if (key == TK_END) selection = menuItemCount;
        if ((key == TK_DOWN || key == TK_KP_2) && selection < menuItemCount) ++selection;

        if (key == TK_MOUSE_LEFT) {
            // int mx = terminal_state(TK_MOUSE_X);
            int my = terminal_state(TK_MOUSE_Y);
            int pos = my - 2 + top;
            if (pos >= 0 && pos <= menuItemCount) selection = pos;
            // for (const UIRect &rect : mouseRegions) {
                // if (mx < rect.x || mx >= rect.x + rect.w) continue;
                // if (my < rect.y || my >= rect.y + rect.h) continue;
                // selection = rect.ident;
                // key = TK_SPACE;
                // break;
            // }
        }

        if (key == TK_CLOSE) {
            return GameReturn::FullQuit;
        }
        if (key == TK_SPACE || key == TK_ENTER || key == TK_KP_ENTER || key == TK_ESCAPE) {
            break;
        }
    }

    return GameReturn::Normal;
}
