#ifndef _TGA_IMAGE_
#define _TGA_IMAGE_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <inttypes.h>

// data type field
typedef enum tga_dtc {
    TGA_DTC_ZERO       = 0,
    TGA_DTC_MAP        = 1,
    TGA_DTC_RGB        = 2,
    TGA_DTC_BW         = 3,
    TGA_DTC_MAP_RLE    = 9,
    TGA_DTC_RGB_RLE    = 10,
    TGA_DTC_BW_RLE     = 11,
    TGA_DTC_MAP_HDRLE  = 32,
    TGA_DTC_MAP_HDRLE4 = 33
} tga_dtc_t;

#define TGA_HEADER_SIZE 18
#define TGA_FOOTER_SIZE 26
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
    uint8_t       bytespp;
} tga_image_t;

uint8_t*     tga_rle_encode  (uint8_t* data, size_t* size, int16_t width, int16_t height, uint8_t bytespp);
uint8_t*     tga_rle_decode  (uint8_t* data, size_t size, int16_t width, int16_t height, uint8_t bytespp);
uint8_t*     tga_endian_swap (uint8_t* data, size_t size, uint8_t bytespp);
uint8_t*     tga_image_getc  (tga_image_t* image);
void         tga_image_setc  (tga_image_t* image, uint8_t* color);
tga_image_t* tga_image_create(tga_header_t header);
tga_image_t* tga_image_read  (const char* filename);
int          tga_image_write (tga_image_t* image, const char* filename);
int          tga_image_rle   (tga_image_t* image);
int          tga_image_rld   (tga_image_t* image);
int          tga_image_map   (tga_image_t* image);
int          tga_image_unmap (tga_image_t* image);
void         tga_image_delete(tga_image_t* image);
uint8_t*     tga_color_get   (uint8_t* data, int16_t width, int16_t height, uint8_t bytespp, int16_t x, int16_t y);
void         tga_color_set   (uint8_t* data, int16_t width, int16_t height, uint8_t bytespp, int16_t x, int16_t y, uint8_t* color);

#ifdef TGA_IMAGE_IMPL
uint8_t* tga_rle_encode(uint8_t* data, size_t* size, int16_t width, int16_t height, uint8_t bytespp) {
    if (data == NULL ||  width < 1 || height < 1 || bytespp == 0 || bytespp > 4) return NULL;
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
            if (countr) { i -= bytespp; countr -= 1;
                buffer[buffer_count] = countr;
                memcpy(buffer + (buffer_count + 1), data + i - ((countr + 1) * bytespp), (countr + 1) * bytespp);
                buffer_count += 1 + (countr + 1) * bytespp;
                countr = 0;
            } else { count++; memcpy(pixelp, data + i, bytespp); }
        } else { 
            if (count) {
                buffer[buffer_count] = count + 128;
                memcpy(buffer + (buffer_count + 1), pixelp, bytespp);
                buffer_count += 1 + bytespp; 
                count = 0;
            } else { countr++; memcpy(pixelp, pixelc, bytespp); }
        } 
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
    uint8_t* return_buff = (uint8_t*)malloc(buffer_count);
    memcpy(return_buff, buffer, buffer_count); free(buffer);
    return return_buff;
}

uint8_t* tga_rle_decode(uint8_t* data, size_t size, int16_t width, int16_t height, uint8_t bytespp) {
    if (data == NULL || size == 0 || width <= 0 || height <= 0 || bytespp == 0 || bytespp > 4) return NULL;
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

uint8_t* tga_endian_swap(uint8_t* data, size_t size, uint8_t bytespp) {
    if (data == NULL || size == 0 || bytespp == 0 || bytespp > 4) return NULL;
    uint8_t* buffer = (uint8_t*)malloc(size);
    memcpy(buffer, data, size);

    if (bytespp > 2) {
        for (size_t i = 0; i < size; i += bytespp) {
            buffer[i]     = data[i + 2];
            buffer[i + 2] = data[i];
        }
    }  return buffer;
}

uint8_t* tga_image_getc(tga_image_t* image) {
    if (image == NULL) return NULL;
    uint8_t* buffer = NULL;

    if (image->header.data_type_code < 9) {
        buffer = tga_endian_swap(image->data_color, image->data_color_size, image->bytespp);
    } else {
        uint8_t* buffer_rld = tga_rle_decode(
            image->data_color, 
            image->data_color_size, 
            image->header.width, image->header.height, 
            image->bytespp
        ); image->data_color_size = 
        image->header.width * 
        image->header.height * 
        image->bytespp
        ; buffer = tga_endian_swap(buffer_rld, image->data_color_size, image->bytespp);
        free(buffer_rld);
    } return buffer;
}

void tga_image_setc(tga_image_t* image, uint8_t* color) {
    if (image == NULL || color == NULL) return;
    if (image->header.data_type_code < 9) {
        uint8_t* color_es = tga_endian_swap(color ,image->data_color_size, image->bytespp);
        memcpy(image->data_color, color_es, image->data_color_size);
        free(color_es);
    } else {
        uint8_t* color_es = tga_endian_swap(color ,image->data_color_size, image->bytespp);
        uint8_t* color_es_rle = tga_rle_encode(
            color_es, 
            &image->data_color_size, 
            image->header.width, image->header.height, 
            image->bytespp
        ); free(image->data_color);
        image->data_color = (uint8_t*)malloc(image->data_color_size); 
        memcpy(image->data_color, color_es_rle, image->data_color_size);
        free(color_es);
        free(color_es_rle);
    }
}

tga_image_t* tga_image_create(tga_header_t header) {
    tga_image_t* image = (tga_image_t*)malloc(sizeof(tga_image_t));
    memset(image, 0, sizeof(tga_image_t));
    image->header  = header;
    image->bytespp = header.bits_per_pixel / 8;

    if (header.id_length) { 
        image->data_id = (uint8_t*)calloc(header.id_length, 1); 
    } if (header.color_map_type) { 
        image->data_map = (uint8_t*)calloc(header.color_map_length * header.color_map_depth, 1); 
    } if (header.data_type_code) { 
        image->data_color_size = header.width * header.height * image->bytespp;
        image->data_color = (uint8_t*)calloc(image->data_color_size, 1); 
        if (header.data_type_code > 3) tga_image_rle(image);
    } return image;
}

tga_image_t* tga_image_read(const char* filename) {
    if (filename == NULL) return NULL;

    FILE *file_stream = fopen(filename, "rb");
	if (file_stream == NULL) return NULL;

    fseek(file_stream, 0, SEEK_END);
    long file_size = ftell(file_stream);
    bool foex = false;
    if (file_size < 18) return NULL;
    uint8_t foot[18];
    uint8_t footer[18] = {'T','R','U','E','V','I','S','I','O','N','-','X','F','I','L','E','.','\0'};
    fseek(file_stream, -17, SEEK_END);
    fread(foot, 1, sizeof(footer), file_stream);
    if (strcmp(foot, footer) != 0) foex = true;
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
    image->bytespp = file_header.bits_per_pixel / 8;
    image->data_color_size = 
        file_size - TGA_HEADER_SIZE - 
        (file_header.color_map_length * file_header.color_map_depth) - 
        file_header.id_length
    ; if (foex) {
        image->data_color_size -= TGA_FOOTER_SIZE;
    } if (file_header.id_length) {
        image->data_id = (uint8_t*)malloc(file_header.id_length);
        fread(image->data_id, 1, file_header.id_length, file_stream);
    } if (file_header.color_map_type) {
        size_t size = file_header.color_map_length * file_header.color_map_depth;
        image->data_map = (uint8_t*)malloc(size);
        fread(image->data_map, 1, size, file_stream);
    } if (file_header.data_type_code > 0) { 
        if (!image->data_color_size) return NULL;
        image->data_color = (uint8_t*)malloc(image->data_color_size);
        fread(image->data_color, 1, image->data_color_size, file_stream);
    } fclose(file_stream);
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

int tga_image_rle(tga_image_t* image) {
    if (image == NULL || image->data_color == NULL || image->header.data_type_code == 0) return false;
    image->header.data_type_code += 8;
    uint8_t* buffer = tga_rle_encode(
        image->data_color, 
        &image->data_color_size, 
        image->header.width, 
        image->header.height, 
        image->bytespp
    ); if (buffer == NULL) return false;
    free(image->data_color);
    image->data_color = (uint8_t*)malloc(image->data_color_size);
    memcpy(image->data_color, buffer, image->data_color_size);
    free(buffer); return true;
}

int tga_image_rld(tga_image_t* image) {
    if (image == NULL || image->data_color == NULL || image->header.data_type_code == 0) return false;
    image->header.data_type_code -= 8;
    uint8_t* buffer = tga_rle_decode(
        image->data_color, 
        image->data_color_size, 
        image->header.width, 
        image->header.height, 
        image->bytespp
    ); if (buffer == NULL) return false;
    free(image->data_color); image->data_color_size = 
        image->header.width *
        image->header.height * 
        image->bytespp
    ; image->data_color = (uint8_t*)malloc(image->data_color_size);
    memcpy(image->data_color, buffer, image->data_color_size);
    free(buffer); return true;
}

void tga_image_delete(tga_image_t* image) {
    if (image == NULL) return;
    free(image->data_id);
    free(image->data_map);
    free(image->data_color);
    free(image);
}

uint8_t* tga_color_get(uint8_t* data, int16_t width, int16_t height, uint8_t bytespp, int16_t x, int16_t y) {
    if (data == NULL || width < 1 || x < 0 || y < 0 || x > width || bytespp == 0 || bytespp > 4) return NULL;
    return data + (x + y * width) * bytespp;
}

void tga_color_set(uint8_t* data, int16_t width, int16_t height, uint8_t bytespp, int16_t x, int16_t y, uint8_t* color) {
    if (data == NULL || width < 1 || x < 0 || y < 0 || x > width || bytespp == 0 || bytespp > 4 || color == NULL) return;
    memcpy(data + (x + y * width) * bytespp, color, bytespp);
}
#endif // TGA_IMAGE_IMPL

#ifdef __cplusplus
}
#endif
#endif // _TGA_IMAGE_