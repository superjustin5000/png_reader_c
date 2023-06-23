#if !defined(PNG_CPP)

#include "common.h"
#include "deflate.cpp"


#pragma pack(push, 1)
struct png_header {
	u8 val1;
	u8 val2;
	u8 val3;
	u8 val4;
	u8 val5;
	u8 val6;
	u8 val7;
	u8 val8;
};

struct png_chunk_header {
	u32 length;
	u32 type;
};

struct png_chunk_footer {
	s32 crc;
};

struct png_ihdr_chunk {
	u32 width;
	u32 height;
	u8 bitDepth;
	u8 colorType;
	u8 compression;
	u8 filter;
	u8 interlace;
};

#pragma pack(pop)







struct Image_Pixel {
	u8 r;
	u8 g;
	u8 b;
	u8 a;
};

struct Image {
	u16 width;
	u16 height;
	Image_Pixel* pixels;
};









u8 PaethPredictor(u8 a, u8 b, u8 c) {
	s32 p = (s32)a + (s32)b - (s32)c;
	s32 pa = p - (s32)a; if (pa < 0) { pa = -pa; }
	s32 pb = p - (s32)b; if (pb < 0) { pb = -pb; }
	s32 pc = p - (s32)c; if (pc < 0) { pc = -pc; }
	if (pa <= pb && pa <= pc) {
		return a;
	}
	else if (pb <= pc) {
		return b;
	}
	return c;
}





void defilter_pass(u8* start, u32 scanline_count, u32 scanline_size, u32 byes_per_complete_pixel) {
    
    for (u32 y = 0; y < scanline_count; y++) {
        
        u32 scanline_start = y * scanline_size;
        u8* scanline       = start + scanline_start;
        u8 filter_byte     = *scanline;
        
        for (u32 x = 1; x < scanline_size; x++) {
            u32 cur_byte_pos = scanline_start + x;
            u8 cur_byte      = *(start + cur_byte_pos);
            
            u8 prev_byte  = 0; 
            u8 prior_byte = 0;
            if (x > byes_per_complete_pixel) {
                prev_byte = *((start + cur_byte_pos) - byes_per_complete_pixel);
            }
            if (y > 0) {
                prior_byte = *((start + cur_byte_pos) - scanline_size);
            }
            
            if (filter_byte == 1) {
                cur_byte += prev_byte;
            }
            else if (filter_byte == 2) {
                cur_byte += prior_byte;
            }
            else if (filter_byte == 3) {
                u8 avg = (prev_byte + prior_byte) >> 1; //floor of divide by 2.
                cur_byte += avg;
            }
            else if (filter_byte == 4) {
                u8 prev_prior_byte = 0;
                if (x > byes_per_complete_pixel && y > 0) {
                    prev_prior_byte = *(((start + cur_byte_pos) - byes_per_complete_pixel) - scanline_size);
                }
                u8 paeth = PaethPredictor(prev_byte, prior_byte, prev_prior_byte);
                cur_byte += paeth;
            }
            *(start + cur_byte_pos) = cur_byte;
            
        }
        
    }
    
}



Image LoadPNG(File_Result file_result) {
	Image img = {};
    
	u8* pngData = (u8*)file_result.contents;
	u8* uncompressed = 0;
	u32 uncompressedSize = 0;
    
	u8 colorType = 6;
    bool interlace = false;
    
	if (file_result.content_size == 0) return img;
    
    png_header* header = ConsumeData(&pngData, png_header);
    
    if (
        header->val1 == 137 &&
        header->val2 == 80 &&
        header->val3 == 78 &&
        header->val4 == 71 &&
        header->val5 == 13 &&
        header->val6 == 10 &&
        header->val7 == 26 &&
        header->val8 == 10) {
        
        
        bool valid = false;
        png_chunk_header* IHDR_HEADER = ConsumeData(&pngData, png_chunk_header);
        EndianSwap(&IHDR_HEADER->length);
        
#define SUPPORT_INTERLACE 1
        
        if (IHDR_HEADER->type == FOURCC("IHDR")) {
            png_ihdr_chunk* IHDR = (png_ihdr_chunk*)ConsumeData(&pngData, png_ihdr_chunk);
            if (
                IHDR->width != 0 &&
                IHDR->height != 0 &&
                IHDR->bitDepth == 8 &&
                (IHDR->colorType == 6 || IHDR->colorType == 2) &&
                IHDR->compression == 0 &&
                IHDR->filter == 0 && 
#if SUPPORT_INTERLACE
                (IHDR->interlace == 0 || IHDR->interlace == 1)
#else
                IHDR->interlace == 0
#endif
                )
            {
                colorType = IHDR->colorType;
                interlace = IHDR->interlace == 1;
                valid = true;
                
                EndianSwap(&IHDR->width);
                EndianSwap(&IHDR->height);
                img.width = (u16)IHDR->width;
                img.height = (u16)IHDR->height;
                
                if (interlace) {
                    Assert(img.width >= 8 && img.height >= 8);
                }
                
                u32 sizeofPixelData = sizeof(Image_Pixel) * img.width * img.height;
                img.pixels = (Image_Pixel*)malloc(sizeofPixelData);
                uncompressed = (u8*)malloc(sizeofPixelData*100);
            }
            
            ConsumeData(&pngData, png_chunk_footer);
        }
        
        if (valid) {
            
            Deflate_Block_Data* compressedChunkData = 0;
            
            
            int safeCounter = 1000000;
            while (safeCounter--) {
                png_chunk_header* chunkHeader = ConsumeData(&pngData, png_chunk_header);
                u8 h1 = chunkHeader->type >> 24;
                u8 h2 = chunkHeader->type >> 16 & 0x000000ff;
                u8 h3 = chunkHeader->type >> 8 & 0x000000ff;
                u8 h4 = chunkHeader->type & 0x000000ff;
                MY_PRINTF("\n%c%c%c%c", h4, h3, h2, h1);
                if (chunkHeader->type == FOURCC("IEND")) {
                    break;
                }
                
                EndianSwap(&chunkHeader->length);
                
                if (chunkHeader->type == FOURCC("IDAT")) {
                    
                    Deflate_Block_Data* nextCompressedChunkData = (Deflate_Block_Data*)malloc(sizeof(Deflate_Block_Data));
                    nextCompressedChunkData->next = 0;
                    nextCompressedChunkData->byteReadingBits = 8;
                    nextCompressedChunkData->bytesRead = 0;
                    
                    if (!compressedChunkData) {
                        compressedChunkData = nextCompressedChunkData;
                        compressedChunkData->first = compressedChunkData;
                    }
                    else {
                        nextCompressedChunkData->first = compressedChunkData->first;
                        compressedChunkData->next = nextCompressedChunkData;
                        compressedChunkData = compressedChunkData->next;
                    }
                    
                    u32 pngCompressedDataLength = chunkHeader->length;
                    u8* data = (u8*)ConsumeDataSize(&pngData, pngCompressedDataLength);
                    compressedChunkData->sizeBytes = pngCompressedDataLength;
                    compressedChunkData->data = data;
                    compressedChunkData->byteReading = *data;
                    
                }
                else {
                    //consume the chunk, it's not IDAT so we don't care (for now)
                    ConsumeDataSize(&pngData, chunkHeader->length);
                }
                
                ConsumeData(&pngData, png_chunk_footer);
            }
            
            
            
            Deflate_Block_Data *currentData = compressedChunkData->first;
            
            uncompressedSize = deflate_decompress(currentData, uncompressed);
        }
        
        
    }
    
    
    MY_PRINTF("DECOMPRESSED, NOW DE-FILTERING");
    
    u32 bytes_per_complete_pixel = 4;
    if (colorType == 2) {
        bytes_per_complete_pixel = 3;
    }
    
    
    
    u32 scanline_size = (img.width * bytes_per_complete_pixel) + 1;
    if (interlace == 0) {
        
        defilter_pass(uncompressed, img.height, scanline_size, bytes_per_complete_pixel);
        
        for (u32 y = 0; y < img.height; y++) {
            
            u32 scanline_start = y * scanline_size;
            
            for (u32 x = 1; x < scanline_size; x++) {
                
                u32 cur_byte_pos = scanline_start + x;
                u8 cur_byte     = *(uncompressed + cur_byte_pos);
                
                u32 pixel_byte = (x-1) % bytes_per_complete_pixel;
                u32 pixel_pos = (img.width * y) + ((x-1) / bytes_per_complete_pixel);
                if (pixel_byte == 0) {
                    (img.pixels + pixel_pos)->r = cur_byte;
                }
                else if (pixel_byte == 1) {
                    (img.pixels + pixel_pos)->g = cur_byte;
                }
                else if (pixel_byte == 2) {
                    (img.pixels + pixel_pos)->b = cur_byte;
                }
                else if (pixel_byte == 3) {
                    (img.pixels + pixel_pos)->a = cur_byte;
                }
                if (colorType == 2) {
                    (img.pixels + pixel_pos)->a = 255;
                }
            }
        }
        
    }
    else if (interlace == 1) {
        u8 pix_every_col[7] = {8,8,4,4,2,2,1};
        u8 pix_every_row[7] = {8,8,8,4,4,2,2};
        u8 pix_start_col[7] = {0,4,0,2,0,1,0};
        u8 pix_start_row[7] = {0,0,4,0,2,0,1};
        u8* start = uncompressed;
        for (u32 pass = 0; pass < 7; pass++) {
            
            u32 pass_scanline_pixel_count = ((img.width - (pix_start_col[pass] + 1)) / pix_every_col[pass]) + 1;
            u32 pass_scanline_count       = ((img.height - (pix_start_row[pass] + 1)) / pix_every_row[pass]) + 1;
            u32 pass_scanline_size        = (pass_scanline_pixel_count * bytes_per_complete_pixel) + 1;
            
            defilter_pass(start, pass_scanline_count, pass_scanline_size, bytes_per_complete_pixel);
            
            
            for (u32 y = 0; y < pass_scanline_count; y++) {
                
                u32 pixel_pos_y = pix_every_row[pass] * y + pix_start_row[pass];
                
                for (u32 x = 0; x < pass_scanline_pixel_count; x++) {
                    
                    if (x == 0) start++; //increment past the filter byte
                    
                    u32 pixel_pos_x = pix_every_col[pass] * x + pix_start_col[pass];
                    
                    Image_Pixel *pixel = img.pixels + (img.width * pixel_pos_y) + pixel_pos_x;
                    for (u8 pixel_byte = 0; pixel_byte < bytes_per_complete_pixel; pixel_byte++) {
                        u8 cur_byte = *(start);
                        if (pixel_byte == 0) {
                            pixel->r = cur_byte;
                        }
                        else if (pixel_byte == 1) {
                            pixel->g = cur_byte;
                        }
                        else if (pixel_byte == 2) {
                            pixel->b = cur_byte;
                        }
                        else if (pixel_byte == 3) {
                            pixel->a = cur_byte;
                        }
                        start++;
                    }
                    
                    if (bytes_per_complete_pixel == 3) {
                        pixel->a = 255;
                    }
                }
            }
        }
    }
    
    
    
    return img;
    
}






void dataReadTest() {
	Deflate_Block_Data* data = (Deflate_Block_Data*)malloc(sizeof(Deflate_Block_Data));
	data->first = data;
	u32 binData = 103550321;
	data->data = &((u8)binData);
	data->next = 0;
	data->bytesRead = 0;
	data->byteReading = *data->data;
	data->byteReadingBits = 8;
	data->sizeBytes = sizeof(binData);
	for (u32 i = 0; i < 10; i++)
	{
		Deflate_Block_Data* nextData = (Deflate_Block_Data*)malloc(sizeof(Deflate_Block_Data));
		nextData->data = &((u8)binData);
		nextData->next = 0;
		nextData->bytesRead = 0;
		nextData->byteReading = *data->data;
		nextData->byteReadingBits = 8;
		nextData->sizeBytes = sizeof(binData);
		nextData->first = data->first;
		data->next = nextData;
		data = nextData;
	}
    
	data = data->first;
	while (data->next) {
		u32 rev = PeakBitsInReverseForHuffman(data, 15);
		u32 bit = ReadDataBits(&data, 15);
		MY_PRINTF("\nrev=%u norm=%u", rev, bit);
	}
}



int main(int argc, char **args) {
    //TODO write test code here
    File_Result file_result = read_file("test.png");
    if (file_result.content_size) {
        Image img = LoadPNG(file_result);
    }
    
    return 0;
}


#define PNG_CPP
#endif