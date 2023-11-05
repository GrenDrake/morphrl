#include <iostream>
#include <string>
#include "BearLibTerminal.h"


void ui_drawFrame(int x, int y, int w, int h) {
    terminal_clear_area(x, y, w, h);
    for (int dy = 0; dy < h; ++dy) {
        terminal_put(x, y + dy, 0x2502);
        terminal_put(x + w - 1, y + dy, 0x2502);
    }
    for (int dx = 0; dx < w; ++dx) {
        terminal_put(x + dx, y, 0x2500);
        terminal_put(x + dx, y + h - 1, 0x2500);
    }
    terminal_put(x, y, 0x250C);
    terminal_put(x + w - 1, y, 0x2510);
    terminal_put(x, y + h - 1, 0x2514);
    terminal_put(x + w - 1, y + h - 1, 0x2518);

}



void ui_alertBox(const std::string &title, const std::string &message) {
    const int maxWidth = 30;
    dimensions_t dims = terminal_measure_ext(maxWidth, 10, message.c_str());

    const std::string &titleStr = "[U+2524] " + title + " [U+251C]";
    const int screenWidth = terminal_state(TK_WIDTH);
    const int screenHeight = terminal_state(TK_HEIGHT);
    const int boxWidth = dims.height > 1 ? maxWidth + 4 : dims.width + 4;
    const int boxHeight = dims.height + 6;
    const int boxLeft = (screenWidth - boxWidth) / 2;
    const int boxTop = (screenHeight - boxHeight) / 2;
    const int buttonTop = boxTop + boxHeight - 3;
    const int buttonLeft = (screenWidth - 6) / 2;
    color_t bgColor = color_from_argb(255, 0, 0, 0);
    color_t fgColor = color_from_argb(255, 196, 196, 196);

    while (1) {
        terminal_bkcolor(bgColor);
        terminal_color(fgColor);
        ui_drawFrame(boxLeft, boxTop, boxWidth, boxHeight);
        terminal_print_ext(boxLeft, boxTop, boxWidth, 1, TK_ALIGN_CENTER, titleStr.c_str());
        terminal_print_ext(boxLeft + 2, boxTop + 2, maxWidth, 10, TK_ALIGN_DEFAULT, message.c_str());
        terminal_color(bgColor);
        terminal_bkcolor(fgColor);
        terminal_print(buttonLeft, buttonTop, " OKAY ");

        terminal_refresh();
        int key = terminal_read();
        if (key == TK_ESCAPE || key == TK_ENTER || key == TK_SPACE || key == TK_KP_ENTER) return;
        if (key == TK_MOUSE_LEFT) {
            int mx = terminal_state(TK_MOUSE_X);
            int my = terminal_state(TK_MOUSE_Y);
            if (mx >= buttonLeft && mx <= buttonLeft + 5 && my == buttonTop) return;
        }
    }
}


