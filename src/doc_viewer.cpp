#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "physfs.h"
#include "BearLibTerminal.h"
#include "morph.h"


Document::~Document() {
    for (DocumentImage &image : images) {
        if (image.image) delete image.image;
    }
}

void showDocument(const std::string &filename) {
    Document *document = loadDocument(filename);
    if (document) {
        showDocument(document);
        delete document;
    }
}

void showDocument(Document *document) {
    if (!document || (document->lines.empty())) {
        std::cerr << "Tried to show empty document.\n";
        return;
    }

    const color_t textColour = color_from_argb(255, 196, 196, 196);
    const color_t backColour = color_from_argb(255, 0, 0, 0);

    const unsigned linesShown = 24;
    const bool canScroll = document->lines.size() > linesShown;
    unsigned topLine = 0;
    while (1) {
        terminal_color(textColour);
        terminal_bkcolor(backColour);
        terminal_clear();

        for (unsigned i = 0; i < linesShown; ++i) {
            unsigned lineNumber = i + topLine;
            if (lineNumber >= document->lines.size()) continue;
            terminal_print(0, i, document->lines[lineNumber].c_str());
        }
        for (DocumentImage &image : document->images) {
            if (image.image) {
                drawImage(image.x, image.y - topLine, image.image);
            }
        }

        terminal_bkcolor(textColour);
        terminal_color(backColour);
        terminal_clear_area(0, 24, 80, 1);
        if (canScroll) terminal_print(21, 24, "Up/Down to scroll     ESCAPE to return");
        else           terminal_print(32, 24, "ESCAPE to return");

        terminal_refresh();

        int key = terminal_read();
        if (key == TK_MOUSE_SCROLL && canScroll) {
            int amount = terminal_state(TK_MOUSE_WHEEL);
            if (amount < 0 && -amount > static_cast<int>(topLine)) topLine = 0;
            else topLine += amount;
            if (topLine > document->lines.size() - linesShown) {
                topLine = document->lines.size() - linesShown;
            }
        }
        if (canScroll && key == TK_HOME) topLine = 0;
        if (canScroll && key == TK_END) topLine = document->lines.size() - linesShown;
        if (canScroll && key == TK_UP && topLine > 0) --topLine;
        if (canScroll && key == TK_DOWN && topLine < document->lines.size() - linesShown) ++topLine;
        if (key == TK_ESCAPE || key == TK_MOUSE_RIGHT) return;
    }
}

Document* loadDocument(const std::string &filename) {
    std::string rawDocument = readFile(filename);
    std::stringstream docfile(rawDocument);

    Document *doc = new Document;
    std::string line;
    unsigned lineNumber = 0;
    while (std::getline(docfile, line)) {
        ++lineNumber;
        if (!line.empty() && line[0] == '@') {
            auto parts = explodeOnWhitespace(line);
            if (parts[0] == "@image") {
                // @image resources/story1.png 0 60
                if (parts.size() != 4) {
                    std::cerr << filename << ':' << lineNumber;
                    std::cerr << " bad argument count for @image (found ";
                    std::cerr << parts.size() << " expected 4)\n";
                } else {
                    DocumentImage image;
                    image.filename = parts[1];
                    strToInt(parts[2], image.x);
                    strToInt(parts[3], image.y);
                    image.image = loadImage(image.filename);
                    doc->images.push_back(image);
                }
            } else {
                std::cerr << filename << ':' << lineNumber;
                std::cerr << "  unknown directive " << parts[0] << '\n';
            }
        } else doc->lines.push_back(line);
    }
    return doc;
}
