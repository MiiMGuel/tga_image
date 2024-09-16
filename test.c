#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TGA_IMAGE_IMPL
#include "tga_image.h"

int main(int argc, char* argv[]) {
    tga_image_t* image = tga_image_load("out.tga"); image->header.image_descriptor = 40;
    if (image == NULL) printf("[info] NULL\n");
    if (!tga_image_write(image, "put.tga")) printf("[info] p write\n");
    tga_image_delete(image);
    return 0;
}