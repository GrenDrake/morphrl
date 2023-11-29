#include <iostream>
#include <string>
#include <vector>

#include "BearLibTerminal.h"
#include "morph.h"


void doMessageLog(World &world) {
    const color_t black = color_from_argb(255, 0, 0, 0);
    const color_t textColour = color_from_argb(255, 192, 192, 192);

    terminal_color(textColour);
    terminal_bkcolor(black);
    terminal_clear();

    int topMessage = 0;
    while (1) {
        terminal_color(textColour);
        terminal_bkcolor(black);
        terminal_clear();

        int yPos = 24;
        for (unsigned i = world.messages.size() - 1; i < world.messages.size(); --i) {
            const unsigned realIndex = i - topMessage;
            if (realIndex >= world.messages.size()) continue;
            const LogMessage &message = world.messages[realIndex];
            dimensions_t dims = terminal_measure_ext(77, 5, message.text.c_str());
            if (dims.height > 1) yPos -= dims.height - 1;
            terminal_put(0, yPos, '*');
            terminal_print_ext(2, yPos, 77, 5, TK_ALIGN_DEFAULT, message.text.c_str());
            --yPos;
            if (yPos < -4) break;
        }

        terminal_color(black);
        terminal_bkcolor(textColour);
        terminal_clear_area(0, 0, 80, 1);
        terminal_print(18, 0, "UP/DOWN scroll message log     ESCAPE return");

        terminal_refresh();

        int key = terminal_read();
        if (key == TK_DOWN || key == TK_KP_2) if (topMessage > 0) --topMessage;
        if (key == TK_UP || key == TK_KP_8) ++topMessage;
        if (key == TK_ESCAPE || key == TK_CLOSE || key == TK_L) return;
        if (key == TK_HOME) topMessage = world.messages.size() - 1;
        if (key == TK_END) topMessage = 0;

        if (static_cast<unsigned>(topMessage) >= world.messages.size()) topMessage = world.messages.size() - 1;
    }
}
