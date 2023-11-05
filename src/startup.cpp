#include <cstdint>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "BearLibTerminal.h"
#include "morph.h"


enum class DocImageAlign {
    None, Left, Right
};
struct DocPage {
    std::vector<std::string> text;
    std::string imageFilename;
    DocImageAlign align;
    Image *image;
};
struct DocStory {
    std::vector<DocPage> pages;
};


void showDocument(DocStory &story);
void gameloop(World &world);


DocPage storyPage1{
    {
        "",
        "The Great War began nearly 300 years ago. Archmages fought",
        "each other, seeking to destroy their rivals and dominate",
        "the world. Decades later it ended not in victory, but with",
        "the Rending. The very forces of reality were torn apart;",
        "the lands shattered, rocks ran as water, the oceans",
        "hardened like stone, and the newborn Winds of Change ran",
        "rampant over the lands. The Winds twisted and changed",
        "everything they touched.",
        "",
        "Thankfully, the Rendering lasted only a few years and its",
        "aftereffects have diminshed over time. The surivors have",
        "learned to deal with what remains and have blamed the",
        "archmages, causing magery to be shunned and, eventually,",
        "forgotten.",
        "",
        "The city of Sanctuary was established as the last bastion",
        "of hope for humanity (though over the past three",
        "centuries, many such \"last bastions\" have been",
        "discovered). A powerful barrier surronds and protects the",
        "city and its surronding lands from the ravages of the",
        "shattered world. The population within live in relative",
        "safety and freedom from random mutations.",
    }, "resources/story1.png", DocImageAlign::Right
};

DocPage storyPage2{
    {
        "",

        "Although established as a haven for humanity, it is not",
        "unwelcoming of mutates (though it's not particularly",
        "welcoming, either), though they often struggle in a world",
        "not built with them in mind. Most mutants are passer-bys,",
        "travelling merchants and the like.",
        "",
        "Nothing lasts forever and the protective barrier always",
        "fades with time. About once a generation, it's neccesary",
        "for someone to go beyond the barrier and retrieve fresh",
        "samples of the Ethereal Ore that powers the barrier. This",
        "time, you've been selected. You're not the first of your",
        "generation, but hopefully you will be the last. Given a",
        "sword, a potion of regeneration, and some specially",
        "designed leather armour that can adapt itself to",
        "mutations, you set out.",
        "",
        "Fortunately, traces of Ethereal Ore have been found in a",
        "mine not far outside the barrier. Unfortunately, it lies",
        "beneath the remains of a wizard's tower, explaining why it",
        "has not been claimed before. You'll need to make your way",
        "through contamination until you can delve deep enough to",
        "claim it.",
    }, "resources/story2.png", DocImageAlign::Left
};

DocPage creditsPage1{
    {
        "","","","","","","","","","","","","",
        "[font=italic]MorphRL: Delving the Mutagenic Dungeons[/font] was originally created for",
        "[font=italic]Transformation game Jam 2023[/font] (https://itch.io/jam/tf23) by Gren Drake.",
        "",
        "MorphRL was developed using:",
        "    BearLibTerminal    http://foo.wyrd.name/en:bearlibterminal",
        "    libfov             https://github.com/google-code-export/libfov",
        "    DejaVu Fonts       https://dejavu-fonts.github.io/",
        "    stb                https://github.com/nothings/stb",
    }, "resources/logo.png", DocImageAlign::None
};

DocStory gameStory{ { storyPage1, storyPage2 } };
DocStory gameCredits{ { creditsPage1 } };


World* createGame() {
    World *world = new World;

    world->player = new Actor(getActorData(0), -1);
    world->player->isPlayer = true;
    world->player->reset();

    world->map = new Dungeon(1, MAP_WIDTH, MAP_HEIGHT);
    doMapgen(*world->map);

    const Room &startRoom = world->map->getRoomByType(1);
    Coord startPosition;
    do {
        startPosition = startRoom.getPointWithin();
        if (!world->map->isValidPosition(startPosition)) {
            startPosition.x = -1;
            continue;
        }
        const MapTile *tile = world->map->at(startPosition);
        const TileData &td = getTileData(tile->floor);
        if (!td.isPassable || tile->actor) startPosition.x = -1;
    } while (startPosition.x < 0);
    std::cerr << "START " << startPosition << '\n';
    world->map->addActor(world->player, startPosition);
    world->map->doActorFOV(world->player);
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
                    gameloop(*world);
                    ++selection;
                    break; }
                case 1:
                    if (world) {
                        gameloop(*world);
                    } else {
                        ui_alertBox("Error", "No game in progress.");
                    }
                    break;
                case 2:
                    showDocument(gameStory);
                    break;
                case 3:
                    showDocument(gameCredits);
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


void showDocument(DocStory &story) {
    // Image *art = loadImage(imageFilename);
    if (story.pages.empty()) return; // can't show an empty document
    unsigned currentPage = 0;

    while (1) {
        DocPage &page = story.pages[currentPage];
        if (page.image == nullptr && !page.imageFilename.empty()) {
            page.image = loadImage(page.imageFilename);
        }

        int leftMargin = 1;
        if (page.align == DocImageAlign::Left) leftMargin = 21;

        terminal_color(color_from_argb(255, 196, 196, 196));
        terminal_bkcolor(color_from_argb(255, 0, 0, 0));
        terminal_clear();

        for (unsigned i = 0; i < page.text.size() && i < 25; ++i) {
            terminal_print(leftMargin, i, page.text[i].c_str());
        }
        terminal_bkcolor(color_from_argb(255, 196, 196, 196));
        terminal_color(color_from_argb(255, 0, 0, 0));
        terminal_clear_area(0, 24, 80, 1);
        if (story.pages.size() > 1) {
            terminal_print(1, 24, "<- Change Page ->  ESCAPE to leave  ENTER/SPACE to continue");
            const std::string pageNumberString = "Page " + std::to_string(currentPage + 1) + " of " + std::to_string(story.pages.size());
            terminal_print(79 - pageNumberString.size(), 24, pageNumberString.c_str());
        } else {
            terminal_print(20, 24, "ESCAPE to leave  ENTER/SPACE to continue");
        }

        if (page.align == DocImageAlign::Right) drawImage(60, 0, page.image);
        else                                    drawImage(0, 0, page.image);
        terminal_refresh();

        int key = terminal_read();
        if ((key == TK_LEFT || key == TK_MOUSE_RIGHT) && currentPage > 0) --currentPage;
        if (key == TK_SPACE || key == TK_ENTER || key == TK_MOUSE_LEFT) {
            if (currentPage < story.pages.size() - 1) ++currentPage;
            else return;
        }
        if (key == TK_RIGHT && currentPage < story.pages.size() - 1) ++currentPage;
        if (key == TK_ESCAPE) return;
    }
}

