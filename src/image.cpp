#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <iostream>

#include "stb_image.h"
#include "stb_image_write.h"

#include "morph.h"
#include "BearLibTerminal.h"


Image::~Image() {
    if (pixels) stbi_image_free(pixels); 
}

Color Image::at(int x, int y) const {
    unsigned pos = (x + y * w) * 3;
    Color c;
    c.r = pixels[pos];
    c.g = pixels[pos + 1];
    c.b = pixels[pos + 2];
    return c;
}

Image* loadImage(const std::string &filename) {
    Image *image = new Image;
    int channels_in_file;
    image->pixels = stbi_load(filename.c_str(), &image->w, &image->h, &channels_in_file, 3);
    if (!image->pixels) {
        std::cerr << "Failed to load image " << filename << ": ";
        std::cerr << stbi_failure_reason() << '\n';
        delete image;
        return nullptr;
    }
    return image;
}

void drawImage(int originX, int originY, Image *image) {
    if (!image) return;

    for (int y = 0; y < image->h; y += 2) {
        for (int x = 0; x < image->w; ++x) {
            Color top = image->at(x, y);
            Color bottom = image->at(x, y + 1);
            terminal_color(color_from_argb(255, top.r, top.g, top.b));
            terminal_bkcolor(color_from_argb(255, bottom.r, bottom.g, bottom.b));
            terminal_put(originX + x, originY + y / 2, 0x2580);
        }
    }
}

static unsigned datapos(int x, int y, int w) {
    return (x + y * w) * 3;
}
void debug_saveMapToPNG(const Dungeon &d, bool showActors) {
    unsigned char *mapdata = new unsigned char[d.width() * d.height() * 3];
    unsigned pos = 0;
    for (int y = 0; y < d.height(); ++y) {
        for (int x = 0; x < d.width(); ++x, ++pos) {
            unsigned pos = datapos(x,y,d.width());
            const MapTile *tile = d.at(Coord(x, y));
            if (showActors && tile->actor) {
                if (tile->actor->data.ident == 0) {
                    mapdata[pos  ] = 127;
                    mapdata[pos+1] = 255;
                    mapdata[pos+2] = 127;
                } else {
                    mapdata[pos  ] = 255;
                    mapdata[pos+1] = 127;
                    mapdata[pos+2] = 127;
                }
            } else {
                const TileData &td = getTileData(tile->floor);
                mapdata[pos  ] = td.r;
                mapdata[pos+1] = td.g;
                mapdata[pos+2] = td.b;
            }
        }
    }
    stbi_write_png("map.png", d.width(), d.height(), 3, mapdata, 0);
    delete[] mapdata;
}
