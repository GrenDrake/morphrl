#include <iostream>
#include <string>
#include <vector>

#include "BearLibTerminal.h"
#include "morph.h"



int keycodeToIndex(int keyCode) {
    switch(keyCode) {
        case TK_1:  return 0;
        case TK_2:  return 1;
        case TK_3:  return 2;
        case TK_4:  return 3;
        case TK_5:  return 4;
        case TK_6:  return 5;
        case TK_7:  return 6;
        case TK_8:  return 7;
        case TK_9:  return 8;
        case TK_0:  return 9;

        case TK_A:  return 0;
        case TK_B:  return 1;
        case TK_C:  return 2;
        case TK_D:  return 3;
        case TK_E:  return 4;
        case TK_F:  return 5;
        case TK_G:  return 6;
        case TK_H:  return 7;
        case TK_I:  return 8;
        case TK_J:  return 9;
        case TK_K:  return 10;
        case TK_L:  return 11;
        case TK_M:  return 12;
        case TK_N:  return 13;
        case TK_O:  return 14;
        case TK_P:  return 15;
        case TK_Q:  return 16;
        case TK_R:  return 17;
        case TK_S:  return 18;
        case TK_T:  return 19;
        case TK_U:  return 20;
        case TK_V:  return 21;
        case TK_W:  return 22;
        case TK_X:  return 23;
        case TK_Y:  return 24;
        case TK_Z:  return 25;
        default:    return -1;
    }
}


Item* selectInventoryItem(World &world, const std::string &prompt) {
    const color_t black = color_from_argb(255, 0, 0, 0);
    const color_t textColour = color_from_argb(255, 192, 192, 192);

    const int invWidth = 60;
    const int invHeight = 20;
    const int leftSide = (80 - invWidth) / 2;
    const int topSide = (24 - invHeight) / 2;
    const int titleOffset = (invWidth - prompt.size()) / 2;
    const int maxItemsListed = invHeight - 8;

    terminal_color(textColour);
    terminal_bkcolor(black);
    for (int x = 0; x <= invWidth; ++x) {
        terminal_put(leftSide + x, topSide, '*');
        terminal_put(leftSide + x, topSide + invHeight, '-');
    }
    for (int y = 0; y <= invHeight; ++y) {
        terminal_put(leftSide,            topSide + y, '|');
        terminal_put(leftSide + invWidth, topSide + y, '|');
    }
    terminal_put(leftSide, topSide, '+');
    terminal_put(leftSide + invWidth, topSide, '+');
    terminal_put(leftSide, topSide + invHeight, '+');
    terminal_put(leftSide + invWidth, topSide + invHeight, '+');
    
    terminal_put(leftSide + titleOffset - 1, topSide, ' ');
    terminal_put(leftSide + titleOffset + prompt.size(), topSide, ' ');
    terminal_bkcolor(textColour);
    terminal_color(black);
    terminal_print(leftSide + titleOffset, topSide, prompt.c_str());
    terminal_color(textColour);
    terminal_bkcolor(black);

    if (world.player->inventory.empty()) {
        terminal_clear_area(leftSide + 1, topSide + 1, invWidth - 1, invHeight - 1);
        terminal_print(leftSide + 2, topSide + 2, "You are carrying nothing.");
        terminal_print(leftSide + 2, topSide + 4, "Press a key to cancel.");
        terminal_refresh();
        terminal_read();
        return nullptr;
    }

    const int curBulk = world.player->getStat(STAT_BULK);
    const int maxBulk = world.player->getStat(STAT_BULK_MAX);
    const std::string bulkString = "Carried Bulk: " + std::to_string(curBulk) + 
                                   " of " + std::to_string(maxBulk);
    const int maxSelection = world.player->inventory.size() - 1;
    int selection = 0;
    while (1) {
        terminal_color(textColour);
        terminal_bkcolor(black);
        terminal_clear_area(leftSide + 1, topSide + 1, invWidth - 1, invHeight - 1);
        terminal_print(leftSide + 5, topSide + 2, "ITEM");
        terminal_print(leftSide + 32, topSide + 2, "BULK");
        terminal_print(leftSide + 3, topSide + invHeight - 2, bulkString.c_str());
        
        int counter = 0;
        int xPos = leftSide + 2;
        int yPos = topSide + 4;
        for (const Item *item : world.player->inventory) {
            if (counter > maxItemsListed) break;
            if (counter == selection) {
                terminal_bkcolor(textColour);
                terminal_color(black);
            } else {
                terminal_color(textColour);
                terminal_bkcolor(black);
            }
            terminal_put(xPos, yPos, 'a' + counter);
            terminal_put(xPos + 1, yPos, ')');
            terminal_print(xPos + 3, yPos, ucFirst(item->getName()).c_str());
            terminal_print(xPos + 30, yPos, std::to_string(item->data.bulk).c_str());
            if (item->isEquipped) {
                terminal_print(xPos + 40, yPos, "(equipped)");
            }
            ++yPos; ++counter;
        }
        terminal_refresh();

        int key = terminal_read();
        if (key == TK_ESCAPE || key == TK_CLOSE) return nullptr;
        if (key == TK_ENTER || key == TK_SPACE) {
            if (selection >= 0 && selection <= maxSelection) {
                return world.player->inventory[selection];
            }
        }
        if (key == TK_UP) {
            if (selection > 0) --selection;
        }
        if (key == TK_DOWN) {
            if (selection < maxSelection && selection < maxItemsListed) ++selection;
        }
        int code = keycodeToIndex(key);
        if (code >= 0) {
            if (code <= maxSelection) selection = code;
        }
    }
}
