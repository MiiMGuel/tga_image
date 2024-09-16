#ifndef _TGA_IMAGE_
#define _TGA_IMAGE_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <inttypes.h>

#define TGA_HEADER_RGB(w, h) (tga_header_t){.data_type_code = TGA_DTF_RGB, .bits_per_pixel = 24, .width = w, .height = h}
#define TGA_HEADER_RGBA(w, h) (tga_header_t){.data_type_code = TGA_DTF_RGB, .bits_per_pixel = 32, .width = w, .height = h}
#define TGA_HEADER_RGB_RLE(w, h) (tga_header_t){.data_type_code = TGA_DTF_RGB_RLE, .bits_per_pixel = 24, .width = w, .height = h}
#define TGA_HEADER_RGBA_RLE(w, h) (tga_header_t){.data_type_code = TGA_DTF_RGB_RLE, .bits_per_pixel = 32, .width = w, .height = h}

// data type field
typedef enum tga_dtf {
    TGA_DTF_ZERO       = 0,
    TGA_DTF_MAP        = 1,
    TGA_DTF_RGB        = 2,
    TGA_DTF_BW         = 3,
    TGA_DTF_MAP_RLE    = 9,
    TGA_DTF_RGB_RLE    = 10,
    TGA_DTF_BW_RLE     = 11,
    TGA_DTF_MAP_HDRLE  = 32,
    TGA_DTF_MAP_HDRLE4 = 33
} tga_dtf_t;

#define TGA_HEADER_SIZE 18
typedef struct tga_header {
    int8_t  id_length;
    int8_t  color_map_type;
    int8_t  data_type_code;
    int16_t color_map_origin;
    int16_t color_map_length;
    int8_t  color_map_depth;
    int16_t x_origin;
    int16_t y_origin;
    int16_t width;
    int16_t height;
    int8_t  bits_per_pixel;
    int8_t  image_descriptor;
} tga_header_t;

typedef struct tga_image {
    tga_header_t header;
    uint8_t*     data_id;
    uint8_t*     data_map;
    uint8_t*     data_color;
    size_t       data_color_size;
} tga_image_t;

uint8_t*     tga_rle_encode  (uint8_t* data, size_t* size, int32_t width, int32_t height, int32_t bytespp);
uint8_t*     tga_rle_decode  (uint8_t* data, size_t size, int32_t width, int32_t height, int32_t bytespp);
tga_image_t* tga_image_load  (const char* filename);
int          tga_image_write (tga_image_t* image, const char* filename);
void         tga_image_delete(tga_image_t* image);

#ifdef TGA_IMAGE_IMPL
uint8_t* tga_rle_encode(uint8_t* data, size_t* size, int32_t width, int32_t height, int32_t bytespp) {
    if (data == NULL ||  width <= 0 || height <= 0 || bytespp <= 0) return NULL;
    size_t   buffer_size  = width * height * bytespp;
    size_t   buffer_count = 0;
    uint8_t* buffer       = (uint8_t*)malloc(buffer_size);
    uint8_t  pixelp[4]    = {0};
    uint8_t  pixelc[4]    = {0};
    uint8_t  count        = 0;
    uint8_t  countr       = 0;
    memcpy(pixelp, data, bytespp);

    for (size_t i = bytespp; i < buffer_size; i += bytespp) {
        memcpy(pixelc, data + i, bytespp);
        if (((memcmp(pixelc, pixelp, bytespp) == 0) || countr > 127) && count < 127) { 
            if (countr) {
                buffer[buffer_count] = countr;
                memcpy(buffer + (buffer_count + 1), data + i - ((countr + 1) * bytespp), (countr + 1) * bytespp);
                buffer_count += 1 + (countr + 1) * bytespp;
                countr = 0;
            } else count++;
        } else if (count) {
            buffer[buffer_count] = count + 128;
            memcpy(buffer + (buffer_count + 1), pixelp, bytespp);
            buffer_count += 1 + bytespp; 
            count = 0;
        } else {
            countr++;
        } memcpy(pixelp, pixelc, bytespp);
    }
    
    if (countr) {
        buffer[buffer_count] = countr;
        memcpy(buffer + (buffer_count + 1), data + (buffer_size - 1) - ((countr + 1) * bytespp), (countr + 1) * bytespp);
        buffer_count += 1 + (countr + 1) * bytespp;
        countr = 0;
    } else if (count) {
        buffer[buffer_count] = count + 128;
        memcpy(buffer + (buffer_count + 1), pixelp, bytespp);
        buffer_count += 1 + bytespp; 
        count = 0;
    }

    *size = buffer_count;
    uint8_t* the_buffer = (uint8_t*)malloc(buffer_count);
    memcpy(the_buffer, buffer, buffer_count); free(buffer);
    return the_buffer;
}

uint8_t* tga_rle_decode(uint8_t* data, size_t size, int32_t width, int32_t height, int32_t bytespp) {
    if (data == NULL || size == 0 || width <= 0 || height <= 0 || bytespp <= 0) return NULL;
    size_t   buffer_size  = width * height * bytespp;
    size_t   buffer_count = 0;
    uint8_t* buffer       = (uint8_t*)malloc(buffer_size);

    for (size_t i = 0; i < size; i++) {
        uint8_t count = data[i];
        if (count < 128) { count++;
            memcpy(buffer + buffer_count, data + i + 1, count * bytespp);
            buffer_count += count * bytespp; 
            i += count * bytespp;
        } else { count -= 127;
            for (uint8_t x = 0; x < count; x++) {
                memcpy(buffer + buffer_count, data + i + 1, bytespp);
                buffer_count += bytespp;
            } i += bytespp;
        }
    }
    
    if (buffer_count != buffer_size) {
        free(buffer);
        return NULL;
    } else return buffer; 
}

tga_image_t* tga_image_load(const char* filename) {
    FILE *file_stream = fopen(filename, "rb");
	if (file_stream == NULL) return NULL;

    bool foex = false;
    uint8_t foot[18];
    uint8_t footer[18] = {'T','R','U','E','V','I','S','I','O','N','-','X','F','I','L','E','.','\0'};
    fseek(file_stream, -17, SEEK_END);
    fread(foot, 1, sizeof(footer), file_stream);
    if (strcmp(foot, footer) != 0) foex = true;
    long file_size = ftell(file_stream);
    fseek(file_stream, 0, SEEK_SET);

    tga_header_t file_header = {0};
    fread(&file_header.id_length,        1, sizeof(int8_t),  file_stream);
    fread(&file_header.color_map_type,   1, sizeof(int8_t),  file_stream);
    fread(&file_header.data_type_code,   1, sizeof(int8_t),  file_stream);
    fread(&file_header.color_map_origin, 1, sizeof(int16_t), file_stream);
    fread(&file_header.color_map_length, 1, sizeof(int16_t), file_stream);
    fread(&file_header.color_map_depth,  1, sizeof(int8_t),  file_stream);
    fread(&file_header.x_origin,         1, sizeof(int16_t), file_stream);
    fread(&file_header.y_origin,         1, sizeof(int16_t), file_stream);
    fread(&file_header.width,            1, sizeof(int16_t), file_stream);
    fread(&file_header.height,           1, sizeof(int16_t), file_stream);
    fread(&file_header.bits_per_pixel,   1, sizeof(int8_t),  file_stream);
    fread(&file_header.image_descriptor, 1, sizeof(int8_t),  file_stream);

    tga_image_t* image = (tga_image_t*)malloc(sizeof(tga_image_t));
    memset(image, 0, sizeof(tga_image_t));
    image->header  = file_header;
    image->data_color_size = 
        file_size - TGA_HEADER_SIZE - 
        (file_header.color_map_length * file_header.color_map_depth) - 
        file_header.id_length
    ; if (foex) {
        image->data_color_size -= ((sizeof(footer) + 8));
    } if (file_header.id_length) {
        image->data_id = (uint8_t*)malloc(file_header.id_length);
        fread(image->data_id, 1, file_header.id_length, file_stream);
    } if (file_header.color_map_type) {
        size_t size = file_header.color_map_length * file_header.color_map_depth;
        image->data_map = (uint8_t*)malloc(size);
        fread(image->data_map, 1, size, file_stream);
    } if (file_header.data_type_code > 0) {
        image->data_color = (uint8_t*)malloc(image->data_color_size);
        fread(image->data_color, 1, image->data_color_size, file_stream);
    }

    fclose(file_stream);
    return image;
}

int tga_image_write(tga_image_t* image, const char* filename) {
    uint8_t developer_area_ref[4] = {0, 0, 0, 0};
	uint8_t extension_area_ref[4] = {0, 0, 0, 0};
	uint8_t footer[18] = {'T','R','U','E','V','I','S','I','O','N','-','X','F','I','L','E','.','\0'};

    FILE *file_stream = fopen(filename, "rb+");
	if (file_stream == NULL) 
		file_stream = fopen(filename, "wb+");
    if (file_stream == NULL)
        return false;
    
    fwrite(&image->header.id_length,        1, sizeof(int8_t),  file_stream);
    fwrite(&image->header.color_map_type,   1, sizeof(int8_t),  file_stream);
    fwrite(&image->header.data_type_code,   1, sizeof(int8_t),  file_stream);
    fwrite(&image->header.color_map_origin, 1, sizeof(int16_t), file_stream);
    fwrite(&image->header.color_map_length, 1, sizeof(int16_t), file_stream);
    fwrite(&image->header.color_map_depth,  1, sizeof(int8_t),  file_stream);
    fwrite(&image->header.x_origin,         1, sizeof(int16_t), file_stream);
    fwrite(&image->header.y_origin,         1, sizeof(int16_t), file_stream);
    fwrite(&image->header.width,            1, sizeof(int16_t), file_stream);
    fwrite(&image->header.height,           1, sizeof(int16_t), file_stream);
    fwrite(&image->header.bits_per_pixel,   1, sizeof(int8_t),  file_stream);
    fwrite(&image->header.image_descriptor, 1, sizeof(int8_t),  file_stream);

    if (image->header.id_length) {
        fwrite(image->data_id, 1, image->header.id_length, file_stream);
    } if (image->header.color_map_type) {
        fwrite(image->data_map, 1, image->header.color_map_length * image->header.color_map_depth, file_stream);
    } if (image->header.data_type_code > 0) {
        fwrite(image->data_color, 1, image->data_color_size, file_stream);
    }

    fwrite(extension_area_ref, 1, 4, file_stream);
    fwrite(developer_area_ref, 1, 4, file_stream);
    fwrite(footer, 1, 18, file_stream);

    fclose(file_stream);
    return true;
}

void tga_image_delete(tga_image_t* image) {
    if (image == NULL) return;
    else if (image->data_color != NULL) free(image->data_color);
    free(image);
}
#endif // TGA_IMAGE_IMPL

#ifdef __cplusplus
}
#endif
#endif // _TGA_IMAGE_