#include <iostream>
#include <string>
#include <vector>

#include "BearLibTerminal.h"
#include "morph.h"


struct UIRect {
    UIRect(int x, int y, int w, int h, int ident)
    : x(x), y(y), w(w), h(h), ident(ident)
    { }

    int x, y, w, h, ident;
};

extern std::vector<ActorData> actorData;
extern std::vector<ItemData> itemData;
extern std::vector<StatusData> statusData;
extern std::vector<MutationData> mutationData;
extern std::vector<TileData> tileData;
extern std::vector<DungeonData> dungeonData;

const int MODE_ACTOR = 0;
const int MAX_MODE = 6;

std::vector<std::string> modeNames {
    "Actors",
    "Items",
    "Tiles",
    "Mutations",
    "Status Effects",
    "Dungeons",
};


template<class T>
void displayEntries(const std::vector<T> &data, unsigned topLine) {
    unsigned nextY = 2;
    for (unsigned i = 0; i < 23; ++i) {
        unsigned thisEntry = topLine + i;
        if (thisEntry >= data.size()) continue;
        terminal_print_ext(0, nextY, 11, 1, TK_ALIGN_RIGHT, std::to_string(data[thisEntry].ident).c_str());
        terminal_print(13, nextY, data[thisEntry].name.c_str());
        ++nextY;
    }
}

void doDebugCodex() {
    color_t textColour = color_from_argb(255, 196, 196, 196);
    color_t backColour = color_from_argb(255, 0, 0, 0);

    unsigned topLine = 0;
    unsigned mode = 0;
    bool done = false;
    std::vector<UIRect> uiRects;
    while (!done) {
        terminal_color(textColour);
        terminal_bkcolor(backColour);
        terminal_clear();

        terminal_bkcolor(textColour);
        terminal_clear_area(0, 0, 80, 1);
        int nextX = 1;
        for (unsigned i = 0; i < MAX_MODE; ++i) {
            if (i != mode) {
                terminal_color(backColour);
                terminal_bkcolor(textColour);
            } else {
                terminal_color(textColour);
                terminal_bkcolor(backColour);
            }

            terminal_print(nextX, 0, modeNames[i].c_str());
            nextX += 2 + modeNames[i].size();
            uiRects.push_back(UIRect(1 + 15* i, 0, 14, 1, static_cast<int>(i)));
        }

        terminal_color(textColour);
        terminal_bkcolor(backColour);
        int nextY = 2;
        switch(mode) {
            case 0: // actors
                displayEntries(actorData, topLine);
                break;
            case 1: // items
                displayEntries(itemData, topLine);
                break;
            case 2: // tiles
                displayEntries(tileData, topLine);
                break;
            case 3: // mutations
                displayEntries(mutationData, topLine);
                break;
            case 4: // status effects
                displayEntries(statusData, topLine);
                break;
            case 5: // dungeons
                displayEntries(dungeonData, topLine);
                break;
        }

        terminal_refresh();

        int key = terminal_read();
        if (key == TK_ESCAPE) return;

        if (key == TK_MOUSE_LEFT) {
            int mx = terminal_state(TK_MOUSE_X);
            int my = terminal_state(TK_MOUSE_Y);
            for (const UIRect &rect : uiRects) {
                if (mx < rect.x || mx >= rect.x + rect.w) continue;
                if (my < rect.y || my >= rect.y + rect.h) continue;
                if (rect.ident < 10) mode = rect.ident;
                break;
            }
        }
        if ((key == TK_LEFT || key == TK_KP_4) && mode > 0) --mode;
        if ((key == TK_RIGHT || key == TK_KP_6) && mode < MAX_MODE - 1) ++mode;
        // if (key != TK_MOUSE_MOVE && key != TK_MOUSE_SCROLL) break;
    }
}