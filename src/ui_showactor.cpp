#include <iostream>
#include <string>
#include <vector>

#include "BearLibTerminal.h"
#include "morph.h"


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