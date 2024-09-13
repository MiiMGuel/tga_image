#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TGA_IMAGE_IMPL
#include "tga_image.h"

int main(int argc, char* argv[]) {
    uint8_t color[4][4] = {
        {0xff, 0xff, 0xff, 0xff},
        {0x00, 0x00, 0xff, 0xff},
        {0x00, 0xff, 0x00, 0xff},
        {0xff, 0x00, 0x00, 0xff}
    };

    tga_image_t* image = tga_image_create(TGA_HEADER_RGBA_RLE(4, 4)); 
    size_t image_size  = image->header.width * image->header.height * (image->header.bits_per_pixel / 8);

    for (size_t y = 1; y <= image->header.height; y++) {
        for (size_t x = 1; x <= image->header.width; x++) {
            tga_image_setp(image, x, y, color[y - 1]);
        }
    }

    tga_image_write(image, "out.tga");
    
    tga_image_delete(image);
    return 0;
}