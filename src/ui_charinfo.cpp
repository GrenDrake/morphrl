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

const char *statDescriptions[] = {
    "Strength allows you to hit harder and also effects how much stuff you can carry with you.",
    "Agility determines your to hit bonus and your evasion.",
    "Speed is how fast you can move; get fast enough, and you can dance circles around your foes.",
    "Toughness is how much of a beating you can tack. It determines your health and makes it easier to resist status afflictions.",
    "Your to hit is a measure of the accuracy of your unarmed and weapon attacks. It's based on your agility.",
    "Evasion is how well you can avoid the attacks of your enemies. It's based on your agility.",
    "Your bulk max is the maximum amount of bulk you can carry with you. It's based on your strength.",
    "Your health is how much of a beating you can take; if it reaches zero, you die. It's based on your toughness.",
    "Your energy is the reserve of power you have available to power special abilities. It's based on your toughness.",
};

void doCharInfo(World &world) {
    const color_t black = color_from_argb(255, 0, 0, 0);
    const color_t textColour = color_from_argb(255, 192, 192, 192);
    const color_t bonus = color_from_argb(255, 127, 127, 255);
    const color_t malus = color_from_argb(255, 255, 127, 127);

    Actor *player = world.player;
    const unsigned modeCount = 4;
    unsigned mode = 0, selection = 0;
    const char *modeNames[modeCount] = {
        "Stats",
        "Mutations",
        "Statuses",
        "Abilities",
    };
    std::vector<unsigned> abilityList = player->getAbilityList();
    std::vector<UIRect> uiRects;
    while (1) {
        uiRects.clear();
        terminal_color(textColour);
        terminal_bkcolor(black);
        terminal_clear();

        terminal_bkcolor(textColour);
        terminal_clear_area(0, 0, 80, 1);
        for (unsigned i = 0; i < modeCount; ++i) {
            if (i != mode) {
                terminal_color(black);
                terminal_bkcolor(textColour);
            } else {
                terminal_color(textColour);
                terminal_bkcolor(black);
            }
            terminal_print(1 + 15 * i, 0, modeNames[i]);
            uiRects.push_back(UIRect(1 + 15* i, 0, 14, 1, static_cast<int>(i)));
        }
        terminal_color(textColour);
        terminal_bkcolor(black);

        int nextY = 2;
        switch(mode) {
            case 0: { // stats
                terminal_print(3, 2, "Health");
                terminal_print(3, 3, "Energy");
                terminal_print(3, 4, "Carried Bulk");
                terminal_print(17, 2, std::to_string(world.player->health).c_str());
                terminal_print(17, 3, std::to_string(world.player->energy).c_str());
                terminal_print(17, 4, std::to_string(player->getStat(STAT_BULK)).c_str());
                terminal_print(41, 2, "Level");
                terminal_print(41, 3, "Experience");
                terminal_print(41, 4, "Advances");
                terminal_print(55, 2, std::to_string(player->level).c_str());
                terminal_print(55, 3, std::to_string(player->xp).c_str());
                terminal_print(55, 4, std::to_string(player->advancementPoints).c_str());

                nextY = 6;
                ++nextY;
                unsigned selectionY = selection;
                // if (selection >= STAT_TO_HIT) ++selectionY;
                if (selection >= STAT_BULK_MAX) ++selectionY;

                terminal_put(1, nextY + selectionY, '*');
                for (unsigned i = 0; i < STAT_BULK; ++i) {
                    // if (i == STAT_TO_HIT) ++nextY;
                    if (i == STAT_BULK_MAX) ++nextY;
                    if (i == selection) {
                        terminal_bkcolor(textColour);
                        terminal_color(black);
                    } else {
                        terminal_color(textColour);
                        terminal_bkcolor(black);
                    }
                    terminal_print(3, nextY, ucFirst(statName(i)).c_str());
                    if (i == selection) {
                        terminal_color(textColour);
                        terminal_bkcolor(black);
                    }
                    terminal_print(17, nextY, std::to_string(player->getStat(i)).c_str());
                    ++nextY;
                }

                terminal_color(textColour);
                if (world.player->isDead()) {
                    terminal_print(0, 20, "You cannot improve stats after dying.");
                } else if (selection >= STAT_BASE_COUNT) {
                    terminal_print(0, 20, "You cannot improve this stat by leveling.");
                } else if (world.player->advancementPoints == 0) {
                    terminal_print(0, 20, "You haven't gained enough experience to improve this stat.");
                } else {
                    terminal_print(0, 20, "Press [color=yellow]SPACE[/color] to improve this stat.");
                }

                int value = 0;
                terminal_color(textColour);
                dimensions_t dims = terminal_print_ext(41, 7, 39, 10, TK_ALIGN_LEFT, statDescriptions[selection]);
                nextY = 8 + dims.height;

                value = player->getStatLevelBonus(selection);
                if (value != 0) {
                    terminal_print(41, nextY, "From levels");
                    if (value > 0) terminal_color(bonus);
                    else if (value < 0) terminal_color(malus);
                    terminal_print(61, nextY, std::to_string(value).c_str());
                    terminal_color(textColour);
                    ++nextY;
                }

                value = player->getStatItemBonus(selection);
                if (value != 0) {
                    terminal_print(41, nextY, "From items");
                    if (value > 0) terminal_color(bonus);
                    else if (value < 0) terminal_color(malus);
                    terminal_print(61, nextY, std::to_string(value).c_str());
                    terminal_color(textColour);
                    ++nextY;
                }

                value = player->getStatStatusBonus(selection);
                if (value != 0) {
                    terminal_print(41, nextY, "From status effects");
                    if (value > 0) terminal_color(bonus);
                    else if (value < 0) terminal_color(malus);
                    terminal_print(61, nextY, std::to_string(value).c_str());
                    terminal_color(textColour);
                    ++nextY;
                }

                value = player->getStatMutationBonus(selection);
                if (value != 0) {
                    terminal_print(41, nextY, "From mutations");
                    if (value > 0) terminal_color(bonus);
                    else if (value < 0) terminal_color(malus);
                    terminal_print(61, nextY, std::to_string(value).c_str());
                }

                break; }

            case 1: // mutations
                if (player->mutations.empty()) {
                    terminal_print(2, nextY, "You have no mutations.");
                } else {
                    unsigned counter = 0;
                    terminal_put(1, nextY + selection, '*');
                    for (const MutationItem *mutation : player->mutations) {
                        if (counter == selection) {
                            terminal_bkcolor(textColour);
                            terminal_color(black);
                        } else {
                            terminal_color(textColour);
                            terminal_bkcolor(black);
                        }
                        terminal_print(3, nextY, ucFirst(mutation->data.name).c_str());
                        if (counter == selection) {
                            terminal_color(textColour);
                            terminal_bkcolor(black);
                        }
                        ++counter;
                        ++nextY;
                    }

                    MutationItem *thisMutation = player->mutations[selection];
                    dimensions_t dims = terminal_print_ext(41, 2, 39, 10, TK_ALIGN_LEFT, thisMutation->data.desc.c_str());
                    nextY = 3 + dims.height;
                    for (unsigned i = 0; i < 10 && i < thisMutation->data.effects.size(); ++i) {
                        terminal_print(41, nextY + i, thisMutation->data.effects[i].toString().c_str());
                    }
                }
                break;

            case 2: // status effects
                if (player->statusEffects.empty()) {
                    terminal_print(2, nextY, "You have no status effects.");
                } else {
                    unsigned counter = 0;
                    terminal_put(1, nextY + selection, '*');
                    for (const StatusItem *status : player->statusEffects) {
                        if (counter == selection) {
                            terminal_bkcolor(textColour);
                            terminal_color(black);
                        } else {
                            terminal_color(textColour);
                            terminal_bkcolor(black);
                        }
                        terminal_print(3, nextY, ucFirst(status->data.name).c_str());
                        if (counter == selection) {
                            terminal_color(textColour);
                            terminal_bkcolor(black);
                        }
                        ++counter;
                        ++nextY;
                    }

                    StatusItem *thisStatus = player->statusEffects[selection];
                    dimensions_t dims = terminal_print_ext(41, 2, 39, 10, TK_ALIGN_LEFT, thisStatus->data.desc.c_str());
                    nextY = 3 + dims.height;
                    for (unsigned i = 0; i < 10 && i < thisStatus->data.effects.size(); ++i) {
                        terminal_print(41, nextY + i, thisStatus->data.effects[i].toString().c_str());
                    }
                }
                break;
            case 3: // abilities
                if (abilityList.empty()) {
                    terminal_print(2, nextY, "You have no special abilities.");
                } else {
                    unsigned counter = 0;
                    terminal_put(1, nextY + selection, '*');
                    for (unsigned abilityId : abilityList) {
                        const AbilityData &thisAbility = getAbilityData(abilityId);
                        if (counter == selection) {
                            terminal_bkcolor(textColour);
                            terminal_color(black);
                        } else {
                            terminal_color(textColour);
                            terminal_bkcolor(black);
                        }
                        if (thisAbility.ident == BAD_VALUE) terminal_print(3, nextY, ("Invalid ability #" + std::to_string(abilityId)).c_str());
                        else terminal_print(3, nextY, ucFirst(thisAbility.name).c_str());
                        if (counter == selection) {
                            terminal_color(textColour);
                            terminal_bkcolor(black);
                        }
                        ++counter;
                        ++nextY;
                    }

                    const AbilityData &thisAbility = getAbilityData(abilityList[selection]);
                    dimensions_t dims = terminal_print_ext(41, 2, 39, 10, TK_ALIGN_LEFT, thisAbility.desc.c_str());
                    nextY = 3 + dims.height;
                    for (unsigned i = 0; i < 10 && i < thisAbility.effects.size(); ++i) {
                        terminal_print(41, nextY + i, thisAbility.effects[i].toString().c_str());
                    }
                }
                break;
        }


        terminal_refresh();
        int key = terminal_read();

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

        if (mode == 0 && (key == TK_SPACE || key == TK_ENTER || key == TK_KP_ENTER)) {
            if (player->isDead() || selection >= STAT_BASE_COUNT || player->advancementPoints < 1) continue;
            --player->advancementPoints;
            ++player->statLevels[selection];
        }

        if (key == TK_CLOSE || key == TK_ESCAPE) return;
        if ((key == TK_LEFT || key == TK_KP_4) && mode > 0) { --mode; selection = 0; }
        if ((key == TK_RIGHT || key == TK_KP_6) && mode < modeCount - 1) { ++mode; selection = 0; }
        if ((key == TK_UP || key == TK_KP_8) && selection > 0) --selection;
        if (key == TK_HOME) selection = 0;
        if ((key == TK_DOWN || key == TK_KP_2) || key == TK_END) {
            unsigned max = 0;
            switch(mode) {
                case 0: max = STAT_BULK - 1; break;
                case 1: max = player->mutations.size() - 1; break;
                case 2: max = player->statusEffects.size() - 1; break;
                case 3: max = abilityList.size() - 1; break;
                default:
                    selection = 0;
            }
            if (selection < max) ++selection;
        }
    }


}