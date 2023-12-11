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
extern std::vector<AbilityData> abilityData;

const int MODE_ACTOR = 0;
const int MAX_MODE = 7;

std::vector<std::string> modeNames {
    "Actors",
    "Items",
    "Tiles",
    "Mutations",
    "Status Effects",
    "Dungeons",
    "Abilities",
};


template<class T>
void displayEntriesWithGlyph(const std::vector<T> &data, unsigned topLine, unsigned selected) {
    const color_t textColour = color_from_argb(255, 196, 196, 196);
    const color_t backColour = color_from_argb(255, 0, 0, 0);
    unsigned nextY = 2;
    for (unsigned i = 0; i < 23; ++i) {
        unsigned thisEntry = topLine + i;
        if (thisEntry >= data.size()) continue;
        if (selected == thisEntry) {
            terminal_bkcolor(textColour);
            terminal_color(backColour);
            terminal_clear_area(0, nextY, 39, 1);
        } else {
            terminal_color(textColour);
            terminal_bkcolor(backColour);
        }
        terminal_print(3, nextY, data[thisEntry].name.c_str());
        terminal_print_ext(28, nextY, 10, 1, TK_ALIGN_RIGHT, std::to_string(data[thisEntry].ident).c_str());
        terminal_color(color_from_argb(255, data[thisEntry].r, data[thisEntry].g, data[thisEntry].b));
        terminal_put(1, nextY, data[thisEntry].glyph);
        ++nextY;
    }
}

template<class T>
void displayEntries(const std::vector<T> &data, unsigned topLine, unsigned selected) {
    const color_t textColour = color_from_argb(255, 196, 196, 196);
    const color_t backColour = color_from_argb(255, 0, 0, 0);
    unsigned nextY = 2;
    for (unsigned i = 0; i < 23; ++i) {
        unsigned thisEntry = topLine + i;
        if (thisEntry >= data.size()) continue;
        if (selected == thisEntry) {
            terminal_bkcolor(textColour);
            terminal_color(backColour);
            terminal_clear_area(0, nextY, 40, 1);
        } else {
            terminal_color(textColour);
            terminal_bkcolor(backColour);
        }
        terminal_print(3, nextY, data[thisEntry].name.c_str());
        terminal_print_ext(28, nextY, 10, 1, TK_ALIGN_RIGHT, std::to_string(data[thisEntry].ident).c_str());
        ++nextY;
    }
}

void showActorData(const ActorData &data) {
    terminal_color(color_from_argb(255, 196, 196, 196));
    terminal_bkcolor(color_from_argb(255, 0, 0, 0));

    unsigned nextY = 2;
    terminal_print(41, nextY, ucFirst(data.name).c_str());
    ++nextY;
    terminal_printf(41, nextY, "Art File: %s", data.artFile.c_str());
    nextY += 2;
    terminal_printf(41, nextY, "Base Level: %d", data.baseLevel);
    ++nextY;
    int statTotal = 0;
    for (int i = 0; i < STAT_BASE_COUNT; ++i) {
        terminal_printf(41, nextY, "%s: %d", ucFirst(statName(i)).c_str(), data.baseStats[i]);
        statTotal += data.baseStats[i];
        ++nextY;
    }
    terminal_printf(41, nextY, "Stat Total: %d", statTotal);
    nextY += 2;
    terminal_print_ext(41, nextY, 39, 20, TK_ALIGN_LEFT, data.desc.c_str());
}
    // std::vector<SpawnLine> initialItems;
    // std::vector<SpawnLine> initialMutations;


void doDebugCodex() {
    color_t textColour = color_from_argb(255, 196, 196, 196);
    color_t backColour = color_from_argb(255, 0, 0, 0);

    unsigned topLine = 0;
    unsigned mode = 0;
    bool done = false;
    std::vector<UIRect> uiRects;
    unsigned selected = 0;
    unsigned selectionMax = 0;
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
            int nameLength = modeNames[i].size();
            uiRects.push_back(UIRect(nextX, 0, nameLength, 1, static_cast<int>(i)));
            nextX += 2 + nameLength;
        }

        terminal_color(textColour);
        terminal_bkcolor(backColour);
        switch(mode) {
            case 0: // actors
                displayEntriesWithGlyph(actorData, topLine, selected);
                selectionMax = actorData.size();
                if (selected < actorData.size()) {
                    showActorData(actorData[selected]);
                }
                break;
            case 1: // items
                displayEntriesWithGlyph(itemData, topLine, selected);
                selectionMax = itemData.size();
                break;
            case 2: // tiles
                displayEntriesWithGlyph(tileData, topLine, selected);
                selectionMax = tileData.size();
                break;
            case 3: // mutations
                displayEntries(mutationData, topLine, selected);
                selectionMax = mutationData.size();
                break;
            case 4: // status effects
                displayEntries(statusData, topLine, selected);
                selectionMax = statusData.size();
                break;
            case 5: // dungeons
                displayEntries(dungeonData, topLine, selected);
                selectionMax = dungeonData.size();
                break;
            case 6: // abilities
                displayEntries(abilityData, topLine, selected);
                selectionMax = abilityData.size();
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
                if (rect.ident < 10) {
                    mode = rect.ident;
                    selected = 0;
                }
                break;
            }
        }
        if ((key == TK_UP || key == TK_KP_8) && selected > 0) --selected;
        if ((key == TK_DOWN || key == TK_KP_2) && selected < selectionMax - 1) ++selected;
        if ((key == TK_LEFT || key == TK_KP_4) && mode > 0) { --mode; selected = 0; }
        if ((key == TK_RIGHT || key == TK_KP_6) && mode < MAX_MODE - 1) { ++mode; selected = 0; }
        // if (key != TK_MOUSE_MOVE && key != TK_MOUSE_SCROLL) break;
    }
}