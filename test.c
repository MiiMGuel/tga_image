#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TGA_IMAGE_IMPL
#include "tga_image.h"

int main(int argc, char* argv[]) {
    tga_image_t* image = NULL;
    if (tga_image_read(&image, "out.tga") > 0) printf("[error] failed reading file!\n");
    uint8_t* buffer; tga_image_decode(image, &buffer);
    tga_image_encode(&image, buffer);
    image->header.image_descriptor = 40;
    if (tga_image_write(image, "put.tga") > 0) printf("[error] failed writing file\n");
    tga_image_delete(image);
    return 0;
}