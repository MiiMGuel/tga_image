#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TGA_IMAGE_IMPL
#include "tga_image.h"

int main(int argc, char* argv[]) {
    uint8_t white[4] = {255, 255, 255, 255};
    uint8_t black[4] = {  0,   0,   0,   0};
    uint8_t red[4]   = {255,   0,   0, 255};
    uint8_t green[4] = {  0, 255,   0, 255};
    uint8_t blue[4]  = {  0,   0, 255, 255};
    
    tga_image_t* image = tga_image_read("images/in.tga");
    if (image == NULL) printf("[error] failed reading file!\n"); 
    if (tga_image_write(image, "images/out.tga") != true) printf("[error] failed writing file\n");
    uint8_t* buffer = tga_image_getc(image);
    tga_color_set(buffer, image->header.width, image->header.height, image->bytespp, 1, 1, white);
    tga_image_setc(image, buffer);
    if (tga_image_rle(image) != true) printf("[error] failed encoding file!\n");
    // if (tga_image_rld(image) != true) printf("[error] failed decoding file!\n");
    if (tga_image_write(image, "images/outc.tga") != true) printf("[error] failed writing file\n");
    tga_image_delete(image);

    image = tga_image_read("images/gs_in.tga");
    if (image == NULL) printf("[error] failed reading file!\n"); 
    if (tga_image_write(image, "images/gs_out.tga") != true) printf("[error] failed writing file\n");
    if (tga_image_rle(image) != true) printf("[error] failed encoding file!\n");
    // if (tga_image_rld(image) != true) printf("[error] failed decoding file!\n");
    if (tga_image_write(image, "images/gs_outc.tga") != true) printf("[error] failed writing file\n");
    tga_image_delete(image);
    return 0;
}