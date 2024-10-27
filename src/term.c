#include "term.h"
#include "nanosvg.h"
#include "nanosvgrast.h"
#include <stdbool.h>
#include <stdlib.h>
#include <wchar.h>

struct rgba {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

typedef struct rgba rgba_t;

bool is_filled(rgba_t *px) {
    return px->a > 200;
}

static char buf[5] = {0};

uint32_t px8braille(rgba_t *pixels, size_t width) {
    uint8_t mask = 
        is_filled(pixels) |
        (is_filled(pixels + width) << 1) |
        (is_filled(pixels + width * 2) << 2) |
        (is_filled(pixels + 1) << 3) |
        (is_filled(pixels + width + 1) << 4) |
        (is_filled(pixels + width * 2 + 1) << 5) |
        (is_filled(pixels + width * 3) << 6) |
        (is_filled(pixels + width * 3 + 1) << 7);


    return 0x2800 | mask;
}


mapchars_t* image_to_chbuf(char const *path, int max_dim) {
    NSVGimage *image = nsvgParseFromFile("../asset/mercator-projection.svg", "px", 69);
    NSVGrasterizer *raster = nsvgCreateRasterizer();

    float scalex = (float)max_dim / (float)image->width;
    float scaley = (float)max_dim / (float)image->height;
    float scale = scalex < scaley ? scalex : scaley;
    int width = scale * image->width;
    int height = scale * image->height;
    rgba_t *rasterized = malloc(sizeof(rgba_t) * width * height);
    nsvgRasterize(raster, image, 0.f, 0.f, scale, (uint8_t*)rasterized, width, height, width * sizeof(rgba_t));

    mapchars_t *map = malloc(sizeof(mapchars_t) + width * height * 3);
    map->width = width / 2;
    map->height = height / 4;

    for(int y = 0; y < height / 4; ++y) {
        for(int x = 0; x < width / 2; ++x) {
            rgba_t *pixel = rasterized + (y * width * 4) + (x * 2);
            if(pixel > rasterized + width * height) {
                exit(-1);
            }

            uint32_t code = px8braille(pixel, width);
            wchar_t *ch = map->data + y * map->width * 3 + x * 3;
            *ch = code;
        }
    }

    nsvgDeleteRasterizer(raster);
    nsvgDelete(image);
    free(rasterized);

    return map;
}
