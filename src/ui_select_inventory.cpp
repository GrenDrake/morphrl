#include <iostream>
#include <string>
#include <vector>

#include "BearLibTerminal.h"
#include "morph.h"


void activateItem(World &world, Item *item, Actor *user) {
    std::stringstream msg;
    bool didEffect = false;
    if (user->isDead()) return;
    if (user == world.player)   msg << "Used ";
    else                        msg << ucFirst(user->getName()) << " used ";
    msg << "[color=yellow]" << item->getName() << "[/color]: ";

    for (const EffectData &data : item->data.effects) {
        if (data.trigger != ET_ON_USE) continue;
        std::string result = triggerEffect(world, data, user, nullptr);
        if (!result.empty()) {
            msg << result;
            didEffect = true;
        }
    }

    if (!didEffect) {
        msg << "no effect. ";
    }
    int roll = globalRNG.upto(100);
    if (item->data.consumeChance > roll) {
        user->removeItem(item);
        if (item->data.consumeChance < 100) {
            msg << "[color=yellow]" << ucFirst(item->getName(true)) << "[/color] was used up.";
        }
        delete item;
    }
    world.addMessage(msg.str());

}


void useItem(World &world, Item *item) {
    if (!item) return;
    if (world.player->isDead()) return;

    std::stringstream msg;
    switch(item->data.type) {
        case ItemData::Weapon: // equip it
            if (item->isEquipped) {
                item->isEquipped = false;
                msg << "Stopped wielding [color=yellow]" << item->getName(true) << "[/color].";
            } else {
                for (Item *itemIter : world.player->inventory) {
                    if (itemIter && itemIter->data.type == ItemData::Weapon && itemIter->isEquipped) {
                        itemIter->isEquipped = false;
                        msg << "Stopped wielding [color=yellow]";
                        msg << itemIter->getName(true) << "[/color]. ";
                    }
                }
                item->isEquipped = true;
                msg << "Now wielding [color=yellow]" << item->getName(true) << "[/color].";
            }
            world.addMessage(msg.str());
            world.tick();
            return;
        case ItemData::Talisman: // equip it
            if (item->isEquipped) {
                item->isEquipped = false;
                msg << "Stopped wearing [color=yellow]" << item->getName(true) << "[/color].";
            } else {
                int talismanCount = world.player->getTalismanCount();
                if (talismanCount < MAX_TALISMANS_WORN) {
                    item->isEquipped = true;
                    msg << "Now wearing [color=yellow]" << item->getName(true) << "[/color].";
                } else {
                    msg << "You're already wearing the maximum number of talismans; remove one to wear [color=yellow]" << item->getName(true) << "[/color].";
                    world.addMessage(msg.str());
                    return;
                }
            }
            world.addMessage(msg.str());
            world.tick();
            return;
        case ItemData::Consumable: { // activate it
            activateItem(world, item, world.player);
            world.tick();
            break; }
        default:
            world.addMessage("[color=yellow]" + ucFirst(item->getName(true)) +
                             "[/color] is not something you can use.");
    }
}

void doInventory(World &world, bool showFloor) {
    const color_t black = color_from_argb(255, 0, 0, 0);
    const color_t textColour = color_from_argb(255, 192, 192, 192);
    const int maxItemsListed = 17;

    terminal_color(textColour);
    terminal_bkcolor(black);
    terminal_clear();

    MapTile *floorTile = world.map->at(world.player->position);
    if (!floorTile) {
        std::cerr << "ERROR: player standing on non-existant floor tile\n";
        return;
    }
    const int maxBulk = world.player->getStat(STAT_BULK_MAX);
    int selection = 0;
    while (1) {
        std::vector<Item*> &currentInventory = showFloor ? floorTile->items : world.player->inventory;
        const int curBulk = world.player->getStat(STAT_BULK);
        const std::string bulkString = "[font=italic]Carried Bulk: " + std::to_string(curBulk) +
                                       " of " + std::to_string(maxBulk);
        const std::string healthString = "[color=red]Health[/color]: " + std::to_string(world.player->health) +
                                         "/" + std::to_string(world.player->getStat(STAT_HEALTH)) +
                                         "    [color=blue]Energy[/color]: " + std::to_string(world.player->energy) +
                                         "/" + std::to_string(world.player->getStat(STAT_ENERGY));
        const char *helpFloor = "[color=yellow]SPACE[/color] pick up";
        const char *helpInventory = "[color=yellow]SPACE[/color] use/equip        [color=yellow]D[/color] drop";
        const int maxSelection = currentInventory.size() < maxItemsListed
                                    ? currentInventory.size() - 1 : maxItemsListed;
        terminal_color(textColour);
        terminal_bkcolor(black);
        terminal_clear();
        // show log (we do this first so we can remove any extra bits that would
        // overlap other UI elements)
        unsigned yPos = 24;
        for (auto iter = world.messages.rbegin(); iter != world.messages.rend(); ++iter) {
            dimensions_t dims = terminal_measure_ext(39, 5, iter->text.c_str());
            if (dims.height > 1) yPos -= dims.height - 1;
            terminal_print_ext(41, yPos, 39, 5, TK_ALIGN_DEFAULT, iter->text.c_str());
            --yPos;
            if (yPos < 17) break;
        }
        terminal_clear_area(41, 0, 39, 17);

        // draw the UI frame
        for (int y = 0; y < 25; ++y) {
            terminal_put(40, y, 0x2502);
        }
        for (int x = 0; x < 40; ++x) {
            terminal_put(x, 20, 0x2500);
            terminal_put(41+x, 16, 0x2500);
        }
        terminal_put(40, 16, 0x251C);
        terminal_put(40, 20, 0x2524);

        if (showFloor)  terminal_print(0, 0, "[font=italic]ITEMS ON [color=yellow]FLOOR");
        else            terminal_print(0, 0, "[font=italic][color=yellow]CARRIED[/color] ITEMS");
        terminal_print(25, 0, "[font=italic]BULK");
        terminal_print(30, 0, "[font=italic]STATUS");
        terminal_print(45, 0, "[font=italic]DESCRIPTION");
        terminal_print(0, 21, "[color=yellow]UP/DOWN[/color] select item    [color=yellow]TAB[/color] change view");
        if (world.player->isDead()) terminal_print(0, 22, "You are [color=red]DEAD[/color].");
        else terminal_print(0, 22, showFloor ? helpFloor : helpInventory);
        terminal_print(0, 23, bulkString.c_str());
        terminal_print(0, 24, healthString.c_str());

        const Item *selectedItem = nullptr;
        int counter = 0;
        yPos = 2;

        if (currentInventory.empty()) {
            terminal_print(0, yPos, "[font=italic]Nothing . . .");
        } else for (const Item *item : currentInventory) {
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
            terminal_print(0, yPos, ucFirst(item->getName()).c_str());
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
        if (key == TK_TAB) {
            showFloor = !showFloor;
            selection = 0;
        }
        if (key == TK_ESCAPE || key == TK_CLOSE || key == TK_I) return;
        if ((key == TK_ENTER || key == TK_SPACE || key == TK_KP_ENTER || (key == TK_G && showFloor)) && !currentInventory.empty() && !world.player->isDead()) {
            if (selection >= 0 && selection <= maxSelection) {
                if (showFloor) {
                    Item *item = currentInventory[selection];
                    world.map->removeItem(item);
                    world.player->addItem(item);
                    world.addMessage("Took [color=yellow]" + item->getName(true) + "[/color].");
                    world.tick();
                } else {
                    Item *item = currentInventory[selection];
                    useItem(world, item);
                }
            }
        }
        if ((key == TK_UP || key == TK_KP_8) && selection > 0) --selection;
        if (key == TK_HOME) selection = 0;
        if ((key == TK_DOWN || key == TK_KP_2) && selection < maxSelection) ++selection;
        if (key == TK_END) selection = maxSelection;
        if (key == TK_D && !showFloor && !world.player->isDead()) {
            if (selection >= 0 && selection <= maxSelection) {
                Item *item = currentInventory[selection];
                world.player->removeItem(item);
                world.map->addItem(item, world.player->position);
                world.addMessage("Dropped [color=yellow]" + item->getName(true) + "[/color].");
                world.tick();
            }
        }

        // verify the current selection is in the valid range
        // this can become invalid if items are dropped or used up
        if (selection > maxSelection) selection = maxSelection;
        if (selection < 0) selection = 0;
    }
}
