#include <cstdint>
#include <ctime>
#include <iostream>
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


void maybeSetHighlight(bool isSet, int x, int y, int w, int h) {
    color_t fgColor = color_from_argb(255, 196, 196, 196);
    color_t bgColor = color_from_argb(255, 0, 0, 0);

    if (isSet) {
        terminal_color(bgColor);
        terminal_bkcolor(fgColor);
    } else {
        terminal_color(fgColor);
        terminal_bkcolor(bgColor);
    }
    if (x >= 0) terminal_clear_area(x, y, w, h);
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
    int which = 0;
    while (!done) {
        mouseRegions.clear();
        terminal_color(fgColor);
        terminal_bkcolor(bgColor);
        terminal_clear();
        terminal_print(0, 0, "[font=italic]MorphRL[/font]: Delving the Mutagenic Dungeon");
        terminal_print(0, 24, "ESC to return    ENTER to rebind    UP / DOWN / LEFT / RIGHT to select binding");

        for (unsigned i = 0; i < keyBindings.size(); ++i) {
            top = selection - maxToShow / 2;
            if (top < 0) top = 0;
            int yLine = i + 2 - top;
            if (yLine < 2) continue;
            if (yLine > maxToShow + 2) break;
            const KeyBinding &binding = keyBindings[i];

            bool thisRow = i == static_cast<unsigned>(selection);
            maybeSetHighlight(thisRow && which == 0, 0, yLine, 14, 1);
            terminal_print(1, yLine, getNameForKey(binding.key[0]).c_str());

            maybeSetHighlight(thisRow && which == 1, 15, yLine, 14, 1);
            if (binding.key[1] != 0) terminal_print(15, yLine, getNameForKey(binding.key[1]).c_str());

            maybeSetHighlight(thisRow && which == 2, 30, yLine, 14, 1);
            if (binding.key[2] != 0) terminal_print(30, yLine, getNameForKey(binding.key[2]).c_str());

            maybeSetHighlight(false, -1, -1, 0, 0);
            if (binding.dir == Direction::Unknown) {
                terminal_print(45, yLine, getNameForAction(binding.action).c_str());
            } else {
                std::stringstream text;
                text << getNameForAction(binding.action) << " " << binding.dir;
                terminal_print(45, yLine, text.str().c_str());
            }
        }

        terminal_refresh();

        int key = terminal_read();
        if (key == TK_HOME) selection = 0;
        if ((key == TK_UP || key == TK_KP_8) && selection > 0) --selection;
        if (key == TK_END) selection = menuItemCount;
        if ((key == TK_DOWN || key == TK_KP_2) && selection < menuItemCount) ++selection;

        if (key == TK_LEFT && which > 0) --which;
        if (key == TK_RIGHT && which < 2) ++which;

        if (key == TK_MOUSE_LEFT) {
            // int mx = terminal_state(TK_MOUSE_X);
            int my = terminal_state(TK_MOUSE_Y);
            int pos = my - 2 + top;
            if (pos >= 0 && pos <= menuItemCount) {
                selection = pos;
            }
        }

        if (key == TK_SPACE || key == TK_ENTER || key == TK_KP_ENTER) {
            if (which == 0) {
                if (keyBindings[selection].action == ACT_FULLQUIT ||
                    keyBindings[selection].action == ACT_MENU)
                    continue;
            }
            terminal_clear_area(0, 24, 80, 1);
            terminal_print(0, 24, "Press new key for binding...");
            terminal_refresh();
            int key = 0;
            do {
                key = terminal_read();
                if (key == TK_ESCAPE) { key = 0; break; }
                if (key == TK_CLOSE) return GameReturn::FullQuit;
                if (getNameForKey(key) == "") key = 0;
            } while (key == 0);
            if (key != 0) keyBindings[selection].key[which] = key;
        }

        if (key == TK_CLOSE) {
            return GameReturn::FullQuit;
        }
        if (key == TK_ESCAPE) {
            break;
        }
    }

    return GameReturn::Normal;
}
