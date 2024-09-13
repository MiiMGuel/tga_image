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
    uint8_t*     data;
} tga_image_t;

uint8_t*     tga_rle_encode  (uint8_t* data, size_t* size, int32_t width, int32_t height, int32_t bytespp);
uint8_t*     tga_rle_decode  (uint8_t* data, size_t size, int32_t width, int32_t height, int32_t bytespp);
tga_image_t* tga_image_create(tga_header_t header);
tga_image_t* tga_image_load  (const char* filename);
bool         tga_image_write (tga_image_t* image, const char* filename);
uint8_t*     tga_image_getp  (tga_image_t* image, int32_t x, int32_t y);
bool         tga_image_setp  (tga_image_t* image, int32_t x, int32_t y, uint8_t* color);
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

tga_image_t* tga_image_create(tga_header_t header) {
    tga_image_t* image = (tga_image_t*)malloc(sizeof(tga_image_t));
    memset(image, 0, sizeof(tga_image_t));
    image->header = header;

    size_t data_size = header.width * header.height * header.bits_per_pixel;
    image->data = (uint8_t*)malloc(data_size);
    memset(image->data, 0, data_size);

    return image;
}

tga_image_t* tga_image_load(const char* filename) {
    FILE *file_stream = fopen(filename, "rb");
	if (file_stream == NULL) return NULL;

    tga_header_t image_header = {0};
    fread(&image_header.id_length,        1, sizeof(int8_t),  file_stream);
    fread(&image_header.color_map_type,   1, sizeof(int8_t),  file_stream);
    fread(&image_header.data_type_code,   1, sizeof(int8_t),  file_stream);
    fread(&image_header.color_map_origin, 1, sizeof(int16_t), file_stream);
    fread(&image_header.color_map_length, 1, sizeof(int16_t), file_stream);
    fread(&image_header.color_map_depth,  1, sizeof(int8_t),  file_stream);
    fread(&image_header.x_origin,         1, sizeof(int16_t), file_stream);
    fread(&image_header.y_origin,         1, sizeof(int16_t), file_stream);
    fread(&image_header.width,            1, sizeof(int16_t), file_stream);
    fread(&image_header.height,           1, sizeof(int16_t), file_stream);
    fread(&image_header.bits_per_pixel,   1, sizeof(int8_t),  file_stream);
    fread(&image_header.image_descriptor, 1, sizeof(int8_t),  file_stream);

    if (
        (image_header.width <= 0) || 
        (image_header.height <= 0) || 
        (image_header.bits_per_pixel > 32) || 
        (image_header.bits_per_pixel < 8)
    ) return NULL; // bad bpp or size value 

    bool foex = false;
    uint8_t foot[18];
    uint8_t footer[18] = {'T','R','U','E','V','I','S','I','O','N','-','X','F','I','L','E','.','\0'};
    fseek(file_stream, -17, SEEK_END);
    fread(foot, 1, sizeof(footer), file_stream);
    if (strcmp(foot, footer) != 0) foex = true;

    int32_t bytespp = image_header.bits_per_pixel / 8;
    size_t image_rdata_size = 0;
    long file_size = ftell(file_stream);
    fseek(file_stream, TGA_HEADER_SIZE, SEEK_SET);
    image_rdata_size = 
        file_size - TGA_HEADER_SIZE - 
        (image_header.color_map_length * image_header.color_map_depth) - 
        image_header.id_length
    ;
    if (foex) {
        image_rdata_size -= ((sizeof(footer) + 8));
    }

    tga_image_t* image = (tga_image_t*)malloc(sizeof(tga_image_t));
    memset(image, 0, sizeof(tga_image_t));
    image->header = image_header;

    if (image_header.data_type_code < 9) {
        size_t image_data_size = image_header.width * image_header.height * bytespp;
        image->data = (uint8_t*)malloc(image_data_size);
        memset(image->data, 0, image_data_size);
        fread(image->data, 1, image_data_size, file_stream);
    } else {
        uint8_t* buffer = (uint8_t*)malloc(image_rdata_size);
        memset(buffer, 0, image_rdata_size);
        fread(buffer, 1, image_rdata_size, file_stream); 
        image->data = tga_rle_decode(
            buffer, 
            image_rdata_size, 
            image_header.width, 
            image_header.height, 
            bytespp
        );
    }

    fclose(file_stream);
    return image;
}

bool tga_image_write(tga_image_t* image, const char* filename) {
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

    if (image->header.data_type_code < 9) {
        size_t image_data_size = image->header.width * image->header.height * (image->header.bits_per_pixel / 8);
        fwrite(image->data, 1, image_data_size, file_stream);
    } else if (image->header.data_type_code < 32) {
        size_t image_data_size = 0;
        uint8_t* image_data_rle = tga_rle_encode(
            image->data, 
            &image_data_size, 
            image->header.width, 
            image->header.height, 
            image->header.bits_per_pixel / 8
        ); fwrite(image_data_rle, 1, image_data_size, file_stream);
        free(image_data_rle);
    }

    // Write the developer area, extension area, and footer
    fwrite(developer_area_ref, 1, 4, file_stream);
    fwrite(extension_area_ref, 1, 4, file_stream);
    fwrite(footer, 1, 18, file_stream);

    fclose(file_stream);
    return true;
}

uint8_t* tga_image_getp(tga_image_t* image, int32_t x, int32_t y) {
    x -= 1; y -= 1;
    if (!image->data || x < 0 || y < 0 || x >= image->header.width || y >= image->header.height) {
		return NULL;
	} return image->data+(x + y * image->header.width) * (image->header.bits_per_pixel / 8);
}

bool tga_image_setp(tga_image_t* image, int32_t x, int32_t y, uint8_t* color) {
    x -= 1; y -= 1;
    if (!image->data || x < 0 || y < 0 || x >= image->header.width || y >= image->header.height || color == NULL) {
		return false;
	} memcpy(
        image->data+(x + y * image->header.width) * (image->header.bits_per_pixel / 8), color, 
        (image->header.bits_per_pixel / 8)
    ); return true;
}

void tga_image_delete(tga_image_t* image) {
    if (image == NULL) return;
    else if (image->data != NULL) free(image->data);
    free(image);
}
#endif // TGA_IMAGE_IMPL

#ifdef __cplusplus
}
#endif
#endif // _TGA_IMAGE_