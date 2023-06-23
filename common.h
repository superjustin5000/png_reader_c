#if !defined(FILE_FORMATS_H)




/*
common typedefs and file platform helper functions
*/

#define _CRT_SECURE_NO_WARNINGS

#include <stdint.h>
#include <stdio.h>
#include <cstring>
#include <malloc.h>

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float f32;
typedef double f64;

#define CRASH *((u32*)0) = 0xDEAD
#define Assert(STATEMENT) if (!(STATEMENT)) CRASH
#define MY_PRINTF




struct File_Result {
    size_t content_size;
    void* contents;
};


File_Result read_file(char* filename) {
    File_Result result = {};
    FILE* f = 0;
    f = fopen(filename, "rb");
    fseek(f, 0L, SEEK_END);
    result.content_size = ftell(f);
    fseek(f, 0L, SEEK_SET);
    result.contents = malloc(result.content_size);
    printf("reading from %s, size: %zu\n", filename, result.content_size);
    size_t bytes_read = fread(result.contents, result.content_size, 1, f);
    if (bytes_read == 0 && ferror(f)) {
        result.content_size = 0;
        result.contents = 0;
    }
    fclose(f);
    return result;
}


void free_file(File_Result result) {
    free(result.contents);
}



bool write_file(char* filename, void* data, u32 size) {
    FILE* f = 0;
    f = fopen(filename, "wb");
    bool result = true;
    if (f) {
        u32 write_count = (u32)fwrite(data, 1, size, f);
        int err = ferror(f);
        if (write_count == 0 && err) {
            result = false;
            perror("error writing\n");
        }
        fclose(f);
    }
    printf("writing %u bytes to %s complete\n", size, filename);
    return result;
}



















void EndianSwap(u32* value) {
	u32 V = (*value);
	V = (
         (V << 24) |
         ((V << 8) & 0xFF0000) |
         ((V >> 8) & 0xFF00) |
         (V >> 24)
         );
	*value = V;
}

#define FOURCC(String) (*(u32*)(String))


#define ConsumeData(data, type) (type*)ConsumeDataSize(data, sizeof(type))
void* ConsumeDataSize(u8** data, u32 size) {
    void* result = (void*)(*data);
	*data = *data + size;
	return result;
}
void* ConsumeDataSizeLong(u8** data, u64 size) {
    void* result = (void*)(*data);
	*data = *data + size;
	return result;
}

#define PushData(dst, src, type) PushDataSize((u8**)dst, (u8*)src, sizeof(type))
u32 PushDataSize(u8** dst, u8* src, u32 size) {
    for (u32 i = 0; i < size; i++) {
        *(*dst) = *src++;
        (*dst)++;
    }
    return size;
}









u32 calc_line_length(char* line_buffer, u32 max_line_size) {
    u32 result = 0;
    for(u32 char_index = 0; char_index < max_line_size; char_index++) {
        result++;
        if (line_buffer[char_index] == '\n') {
            break;
        }
    }
    return result;
}



void copy_line_to_buffer(char* buffer, char* line, u32* index) {
    while(true) {
        u32 i = *index;
        *(buffer + i) = *line;
        *index = i + 1;
        if(*line == '\n') break;
        line++;
    }
}









#define FILE_FORMATS_H
#endif