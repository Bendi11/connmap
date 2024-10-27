#pragma once

#include <stdint.h>
#include <wchar.h>


struct mapchars {
    int width;
    int height;
    wchar_t data[];
};

typedef struct mapchars mapchars_t;

mapchars_t* image_to_chbuf(char const *path, int max_dim);
