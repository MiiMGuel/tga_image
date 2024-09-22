#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TGA_IMAGE_IMPL
#include "tga_image.h"

int main(int argc, char* argv[]) {
    tga_image_t* image = tga_image_read("in.tga");
    if (image == NULL) printf("[error] failed reading file!\n"); 
    if (tga_image_write(image, "out.tga") != true) printf("[error] failed writing file\n");
    if (tga_image_rle(image) != true) printf("[error] failed encoding file!\n");
    // if (tga_image_rld(image) != true) printf("[error] failed decoding file!\n");
    if (tga_image_write(image, "outc.tga") != true) printf("[error] failed writing file\n");
    tga_image_delete(image);
    return 0;
}