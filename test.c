#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TGA_IMAGE_IMPL
#include "tga_image.h"

int main(int argc, char* argv[]) {
    tga_image_t* image = tga_image_read("images/in.tga");
    if (image == NULL) printf("[error] failed reading file!\n"); 
    if (tga_image_write(image, "images/out.tga") != true) printf("[error] failed writing file\n");
    uint8_t* buffer = tga_bgra_rgba(image->data_color, image->data_color_size);
    free(image->data_color); image->data_color = tga_bgra_rgba(buffer, image->data_color_size);
    if (tga_image_rle(image) != true) printf("[error] failed encoding file!\n");
    // if (tga_image_rld(image) != true) printf("[error] failed decoding file!\n");
    if (tga_image_write(image, "images/outc.tga") != true) printf("[error] failed writing file\n");
    tga_image_delete(image); free(buffer);

    image = tga_image_read("images/gs_in.tga");
    if (image == NULL) printf("[error] failed reading file!\n"); 
    if (tga_image_write(image, "images/gs_out.tga") != true) printf("[error] failed writing file\n");
    if (tga_image_rle(image) != true) printf("[error] failed encoding file!\n");
    // if (tga_image_rld(image) != true) printf("[error] failed decoding file!\n");
    if (tga_image_write(image, "images/gs_outc.tga") != true) printf("[error] failed writing file\n");
    tga_image_delete(image);
    return 0;
}