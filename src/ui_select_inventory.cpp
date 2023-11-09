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
    if (world.player->inventory.empty()) {
        ui_alertBox("Alert", "You are not carrying anything.");
        return nullptr;
    }

    const color_t black = color_from_argb(255, 0, 0, 0);
    const color_t textColour = color_from_argb(255, 192, 192, 192);
    const int maxItemsListed = 20;

    terminal_color(textColour);
    terminal_bkcolor(black);
    terminal_clear();

    const int curBulk = world.player->getStat(STAT_BULK);
    const int maxBulk = world.player->getStat(STAT_BULK_MAX);
    const std::string bulkString = "[font=italic]Carried Bulk: " + std::to_string(curBulk) +
                                   " of " + std::to_string(maxBulk);
    const int maxSelection = world.player->inventory.size() - 1;
    int selection = 0;
    while (1) {
        terminal_color(textColour);
        terminal_bkcolor(black);
        terminal_clear();
        terminal_print(3, 0, "[font=italic]ITEM");
        terminal_print(25, 0, "[font=italic]BULK");
        terminal_print(30, 0, "[font=italic]STATUS");
        terminal_print(45, 0, "[font=italic]DESCRIPTION");
        terminal_print(0, 24, bulkString.c_str());

        const Item *selectedItem = nullptr;
        int counter = 0;
        int yPos = 2;
        for (const Item *item : world.player->inventory) {
            if (counter > maxItemsListed) break;
            if (counter == selection) {
                terminal_bkcolor(textColour);
                terminal_color(black);
                terminal_clear_area(0, yPos, 40, 1);
                selectedItem = item;
            } else {
                terminal_color(textColour);
                terminal_bkcolor(black);
            }
            terminal_put(0, yPos, 'a' + counter);
            terminal_put(1, yPos, ')');
            terminal_print(3, yPos, ucFirst(item->getName()).c_str());
            terminal_print(25, yPos, std::to_string(item->data.bulk).c_str());
            if (item->isEquipped) {
                terminal_print(30, yPos, "(equipped)");
            }
            ++yPos; ++counter;
        }

        if (selectedItem) {
            terminal_color(textColour);
            terminal_bkcolor(black);
            int yPos = 2;
            if (!selectedItem->data.desc.empty()) {
                dimensions_t descSize = terminal_print_ext(41, 2, 35, 5, TK_ALIGN_DEFAULT, selectedItem->data.desc.c_str());
                yPos += 1 + descSize.height;
            }
            if (selectedItem->data.type == ItemData::Weapon) {
                terminal_printf(41, yPos, "Damage [color=red]%d[/color] to [color=red]%d[/color]",
                                          selectedItem->data.minDamage, selectedItem->data.maxDamage);
                ++yPos;
            }
            for (unsigned i = 0; i < 10 && i < selectedItem->data.effects.size(); ++i)
            terminal_print(41, yPos + i, selectedItem->data.effects[i].toString().c_str());
        }

        terminal_refresh();

        int key = terminal_read();
        if (key == TK_MOUSE_LEFT) {
            int mx = terminal_state(TK_MOUSE_X);
            int my = terminal_state(TK_MOUSE_Y);
            if (mx > 45 || my < 2 || my > 22) continue;
            my -= 2;
            if (my <= maxSelection) selection = my;
        }
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
