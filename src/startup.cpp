#include <cstdint>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "BearLibTerminal.h"
#include "morph.h"


void gameloop(World &world);


World* createGame() {
    World *world = new World;

    world->player = Actor::create(getActorData(0));
    world->player->isPlayer = true;
    world->player->reset();
    world->movePlayerToDepth(getDungeonEntranceIdent(), DE_ENTRANCE);
    world->addMessage("Welcome to MorphRL!");
    return world;
}

struct UIRect {
    int x, y, w, h, ident;
};

int main() {
    srand(time(nullptr));
    std::cerr << "LOADING DATA\n";
    if (!loadAllData()) return 1;

    terminal_open();
    terminal_set("window.title='MorphRL';");
    terminal_set("input.filter = [keyboard, mouse];");
    terminal_set("font: resources/DejaVuSansMono.ttf, size=24;");
    terminal_set("italic font: resources/DejaVuSansMono-Oblique.ttf, size=24;");

    std::vector<UIRect> mouseRegions;
    World *world = nullptr;
    std::cerr << "ENTERING main menu\n";
    const std::string versionString = "Development Release 1";
    const int versionX = 79 - versionString.size();
    Image *logo = loadImage("resources/logo.png");
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
        terminal_print(versionX, 24, ("[font=italic]" + versionString).c_str());
        terminal_print(8, 16, "Start new game");
        mouseRegions.push_back(UIRect{8, 16, 20, 1, 0});
        terminal_print(8, 17, "Resume current game");
        mouseRegions.push_back(UIRect{8, 17, 20, 1, 1});
        terminal_print(8, 18, "Story");
        mouseRegions.push_back(UIRect{8, 18, 20, 1, 2});
        terminal_print(8, 19, "Credits");
        mouseRegions.push_back(UIRect{8, 19, 20, 1, 3});
        terminal_print(8, 20, "Quit");
        mouseRegions.push_back(UIRect{8, 20, 20, 1, 4});
        terminal_print(5, 16+selection, "->");
        terminal_color(fgColorDark);
        terminal_print(31, 12, "Pre-Alpha Release");
        drawImage(0, 0, logo);
        terminal_refresh();

        int key = terminal_read();
        if (key == TK_ESCAPE || key == TK_Q || key == TK_CLOSE) {
            break;
        }
        if (key == TK_HOME) selection = 0;
        if ((key == TK_UP || key == TK_KP_8) && selection > 0) --selection;
        if (key == TK_END) selection = 4;
        if ((key == TK_DOWN || key == TK_KP_2) && selection < 4) ++selection;

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
                case 0: {
                    // start new game
                    if (world) {
                        std::cerr << "FREEING old game\n";
                        delete world;
                    }
                    world = createGame();
                    if (!world) {
                        ui_alertBox("Error", "Could not create game world.");
                    } else {
                        gameloop(*world);
                        ++selection;
                    }
                    break; }
                case 1:
                    if (world) {
                        gameloop(*world);
                    } else {
                        ui_alertBox("Error", "No game in progress.");
                    }
                    break;
                case 2:
                    showDocument("resources/story.txt");
                    break;
                case 3:
                    showDocument("resources/credits.txt");
                    break;
                case 4:
                    // quit
                    done = true;
                    break;
            }
        }
    }

    if (world) delete world;
    delete logo;
    terminal_close();


    return 0;
}
