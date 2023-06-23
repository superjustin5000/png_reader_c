
u16 deflate_length_codes[29][2] = {
	{0,3},{0,4},{0,5},{0,6},{0,7},{0,8},{0,9},{0,10},
	{1,11},{1,13},{1,15},{1,17},
	{2,19},{2,23},{2,27},{2,31},
	{3,35},{3,43},{3,51},{3,59},
	{4,67},{4,83},{4,99},{4,115},
	{5,131},{5,163},{5,195},{5,227},
	{0, 258}
};

u16 deflate_distance_codes[30][2] = {
	{0, 1}, {0, 2}, {0, 3}, {0, 4}, {1, 5}, {1, 7}, {2, 9}, {2, 13}, {3, 17}, {3, 25},
	{4, 33}, {4, 49}, {5, 65}, {5, 97}, {6, 129}, {6, 193}, {7, 257}, {7, 385}, {8, 513}, {8, 769},
	{9, 1025}, {9, 1537}, {10, 2049}, {10, 3073}, {11, 4097}, {11, 6145}, {12, 8193}, {12, 12289}, {13, 16385}, {13, 24577}
};




//NOTE(justin):structured in blocks in case of files like png that store deflate packets in chunks.

struct Deflate_Block_Data {
    u8* data;
	u32 sizeBytes;
	u8 byteReading;
	u8 byteReadingBits = 8;
	u32 bytesRead = 0;
	Deflate_Block_Data* first;
	Deflate_Block_Data* next;
};



u32 ReadDataBits(Deflate_Block_Data** chunkData, u8 bits) {
	Assert(bits <= 16 && bits > 0);
	Deflate_Block_Data* chunk = *chunkData;
    
	u32 result = 0;
	for (u32 bitIndex = 0; bitIndex < bits; bitIndex++) {
		if (chunk->byteReadingBits == 0) {
			chunk->bytesRead++;
			if (chunk->bytesRead >= chunk->sizeBytes) {
				Assert(chunk->next != 0);
				Assert(chunk->next->data != 0);
				*chunkData = chunk->next;
				chunk = *chunkData;
				chunk->bytesRead = 0;
			}
			else {
				chunk->data = chunk->data + 1;
			}
			chunk->byteReading = *chunk->data;
			chunk->byteReadingBits = 8;
		}
		u8 nextBit = chunk->byteReading & 1;
		u32 bitInPosition = nextBit << bitIndex;
		result |= bitInPosition;
		chunk->byteReading  = chunk->byteReading >> 1;
		chunk->byteReadingBits--;
	}
	return result;
}

u32 PeakBitsInReverseForHuffman(Deflate_Block_Data* chunk, u32 bits) {
	u8* data = chunk->data;
	u32 sizeBytes = chunk->sizeBytes;
	u8 byteReading = chunk->byteReading;
	u8 byteReadingBits = chunk->byteReadingBits;
	u32 bytesRead = chunk->bytesRead;
	u32 result = 0;
	for (u32 bitIndex = 0; bitIndex < bits; bitIndex++) {
		if (byteReadingBits == 0) {
			bytesRead++;
			if (bytesRead >= sizeBytes) {
				Assert(chunk->next != 0);
				Assert(chunk->next->data != 0);
				data = chunk->next->data;
				bytesRead = 0;
				sizeBytes = chunk->next->sizeBytes;
			}
			else {
				data++;
			}
			byteReading = *data;
			byteReadingBits = 8;
		}
		u8 nextBit = byteReading & 1;
		u32 bitInPosition = nextBit << ((bits - bitIndex) - 1);
		result |= bitInPosition;
		byteReading = byteReading >> 1;
		byteReadingBits--;
	}
	return result;
}






#define CODE_LEN_CODE_ALPHA_SIZE 19
#define CODE_LITLEN_ALPHA_SIZE 288
#define CODE_DIST_ALPHA_SIZE 32
#define CODE_ALPHA_SIZE 320
#define LIT_LEN_STOP_CODE 256


void BuildHuffmanCodesFromCodeLengthCounts(u8* codeLengths, u32 size, u32* result) {
	u8 longestCodeLength = 0;
	for (u32 index = 0; index < size; index++) {
		if (longestCodeLength < codeLengths[index]) {
			longestCodeLength = codeLengths[index];
		}
	}
	u32 numCodeLengths = longestCodeLength + 1;
    
	//each index of codeLengthCounts represents a length, i=0 is num codes of length 0.
	u32 codeLengthCounts[CODE_ALPHA_SIZE] = {};
	memset(codeLengthCounts, 0, numCodeLengths * sizeof(u32));
    
	for (u32 index = 0; index < size; index++) {
		u32 lengthIndex = codeLengths[index];
		u32 currentCountOfLength = codeLengthCounts[lengthIndex];
		codeLengthCounts[lengthIndex] = currentCountOfLength + 1;
	}
	codeLengthCounts[0] = 0;
    
	//next codes initialized with the first code of each length, length being the index.
	u32 nextCodes[CODE_ALPHA_SIZE] = {};
	memset(nextCodes, 0, numCodeLengths * sizeof(u32));
	u32 code = 0;
	for (u32 bitLength = 1; bitLength <= longestCodeLength; bitLength++) {
		int countOfPrevCodeLength = codeLengthCounts[bitLength - 1];
		code = (code + countOfPrevCodeLength) << 1;
		nextCodes[bitLength] = code;
	}
    
	//we have to assign code to every element in alphabet even if we have to assign 0.
	//uint32* assignedCodes = (uint32*)malloc(size * sizeof(uint32));
	for (u32 index = 0; index < size; index++) {
		u32 codeLength = codeLengths[index];
		if (codeLength != 0) {
			u32 nextCodeOfThisLength = nextCodes[codeLength];
			result[index] = nextCodeOfThisLength;
			//increment the prev code of the length so the next assignment to the same length has 1 higher value.
			nextCodes[codeLength] = nextCodeOfThisLength + 1;
		}
	}
}


bool DecodeHuffmanSymbol(Deflate_Block_Data **data, u32* codes, u8* codeLengths, u32 numSymbols, u32 *decodedSymbol) {
	bool found = false;
	for (u32 symbolIndex = 0; symbolIndex < numSymbols; symbolIndex++) {
		u8 bitsToRead = codeLengths[symbolIndex];
		if (bitsToRead == 0) {
			continue;
		}
		u32 code = PeakBitsInReverseForHuffman(*data, bitsToRead);
		if (codes[symbolIndex] == code) {
			*decodedSymbol = symbolIndex;
			found = true;
			ReadDataBits(data, bitsToRead);
			break;
		}
	}
	return found;
}







//NOTE(justin) uncompressd size is precalculated depending on the desired output data format.
u32 deflate_decompress(Deflate_Block_Data* currentData, u8* uncompressed) {
    
    //NOTE(justin): this is more specific to zlib and not deflate in general.
    u8 zLibMethodFlags = (u8)ReadDataBits(&currentData, 8);
    u8 zLibAdditionalFlags = (u8)ReadDataBits(&currentData, 8);
    u8 CM = zLibMethodFlags & 0xF;
    u8 CINFO = zLibMethodFlags >> 4;
    u8 FCHECK = zLibAdditionalFlags & 0b11111;
    u8 FDICT = zLibAdditionalFlags & 0b00100000;
    u8 FLEVEL = zLibAdditionalFlags >> 6;
    
    
    u32 uncompressedSize = 0;
    u32 blockCount = 1;
    while(true) {
        
        u32 BFINAL = ReadDataBits(&currentData, 1);
        u32 BTYPE = ReadDataBits(&currentData, 2);
        
        
        
        if (BTYPE == 0) {
            /*
              skip any remaining bits in current partially
               processed byte
              read LEN and NLEN (see next section)
              copy LEN bytes of data to output
              */
            if (currentData->byteReadingBits) {
                ReadDataBits(&currentData, currentData->byteReadingBits);
            }
            u32 LEN1 = ReadDataBits(&currentData, 8);
            u32 LEN2 = ReadDataBits(&currentData, 8);
            u32 LEN = (LEN1 << 8) | LEN2;
            
            ReadDataBits(&currentData, 16); //NLEN bits.
            
            for (u32 i = 0; i < LEN; i++) {
                *(uncompressed + uncompressedSize++) = (u8)ReadDataBits(&currentData, 8);
            }
        }
        else if (BTYPE == 1 || BTYPE == 2) {
            u32 HLIT = 257;
            u32 HDIST = 1;
            u32 HCLEN = 4;
            
            u32 litLenHuffTree[CODE_LITLEN_ALPHA_SIZE] = {};
            u32 distHuffTree[CODE_DIST_ALPHA_SIZE] = {};
            u8 decodedCodeLengths[CODE_ALPHA_SIZE] = {};
            
            if (BTYPE == 2) {
                HLIT += ReadDataBits(&currentData, 5);
                HDIST += ReadDataBits(&currentData, 5);
                HCLEN += ReadDataBits(&currentData, 4);
                
                
                u8 codeLengthIndexOrder[] = { 16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15 };
                
                u8 codeLengths[CODE_LEN_CODE_ALPHA_SIZE] = {}; //size is element count of array above.
                memset(codeLengths, 0, CODE_LEN_CODE_ALPHA_SIZE);
                for (u8 codeLengthIndex = 0; codeLengthIndex < HCLEN; codeLengthIndex++) {
                    u32 codeLength = ReadDataBits(&currentData, 3);
                    u8 realIndex = codeLengthIndexOrder[codeLengthIndex];
                    codeLengths[realIndex] = (u8)codeLength;
                }
                
                u32 huffCodes[CODE_LEN_CODE_ALPHA_SIZE];
                BuildHuffmanCodesFromCodeLengthCounts(codeLengths, CODE_LEN_CODE_ALPHA_SIZE, huffCodes);
                
                
                u32 codeIndex = 0;
                while (codeIndex < (HLIT + HDIST)) {
                    //alphabet symbol is one of 0-18 from above.
                    u32 codeLengthAlphabetSymbol = 0;
                    bool success = DecodeHuffmanSymbol(&currentData, huffCodes, codeLengths, CODE_LEN_CODE_ALPHA_SIZE, &codeLengthAlphabetSymbol);
                    if (!success) {
                        MY_PRINTF("DECODE HUFFMAN FAILED FOR CODE INDEX " + codeIndex);
                        CRASH;
                    }
                    
                    if (codeLengthAlphabetSymbol < 16) {
                        decodedCodeLengths[codeIndex] = (u8)codeLengthAlphabetSymbol;
                        codeIndex++;
                    }
                    else {
                        u32 repeatCount = 0;
                        u8 codeLengthToRepeat = 0;
                        if (codeLengthAlphabetSymbol == 16) {
                            repeatCount = ReadDataBits(&currentData, 2) + 3;
                            codeLengthToRepeat = decodedCodeLengths[codeIndex - 1];
                        }
                        else if (codeLengthAlphabetSymbol == 17) {
                            repeatCount = ReadDataBits(&currentData, 3) + 3;
                        }
                        else if (codeLengthAlphabetSymbol == 18) {
                            repeatCount = ReadDataBits(&currentData, 7) + 11;
                        }
                        
                        for (u32 repeatIndex = 0; repeatIndex < repeatCount; repeatIndex++) {
                            decodedCodeLengths[codeIndex] = codeLengthToRepeat;
                            codeIndex++;
                        }
                    }
                }
                
                
            }
            
            else if (BTYPE == 1) {
                HLIT = CODE_LITLEN_ALPHA_SIZE;
                for (u32 lengthIndex = 0; lengthIndex < HLIT; lengthIndex++) {
                    if (lengthIndex >= 280) {
                        decodedCodeLengths[lengthIndex] = 8;
                    }
                    else if (lengthIndex >= 256) {
                        decodedCodeLengths[lengthIndex] = 7;
                    }
                    else if (lengthIndex >= 144) {
                        decodedCodeLengths[lengthIndex] = 9;
                    }
                    else {
                        decodedCodeLengths[lengthIndex] = 8;
                    }
                }
                HDIST = CODE_DIST_ALPHA_SIZE;
                for (u32 lengthIndex = 0; lengthIndex < HDIST; lengthIndex++) {
                    decodedCodeLengths[HLIT + lengthIndex] = 5;
                }
            }
            
            
            BuildHuffmanCodesFromCodeLengthCounts(decodedCodeLengths, HLIT, litLenHuffTree);
            BuildHuffmanCodesFromCodeLengthCounts(decodedCodeLengths + HLIT, HDIST, distHuffTree);
            
            while (true) {
                
                u32 lengthCode = 0;
                bool success = DecodeHuffmanSymbol(&currentData, litLenHuffTree, decodedCodeLengths, HLIT, &lengthCode);
                if (!success) {
                    MY_PRINTF("DECODE HUFFMAN FAILED FOR LENGTH CODE");
                    CRASH;
                }
                
                Assert(lengthCode < CODE_LITLEN_ALPHA_SIZE-2);
                if (lengthCode == LIT_LEN_STOP_CODE) {
                    break;
                }
                else if (lengthCode < LIT_LEN_STOP_CODE) {
                    *(uncompressed + uncompressedSize++) = (u8)lengthCode;
                }
                else if (lengthCode > LIT_LEN_STOP_CODE) {
                    u32 lengthIndex = lengthCode - (LIT_LEN_STOP_CODE + 1);
                    u16 lengthExtra = deflate_length_codes[lengthIndex][0];
                    u16 length = deflate_length_codes[lengthIndex][1];
                    if (lengthExtra > 0) {
                        Assert(lengthExtra <= 5);
                        u32 extraBitsValue = ReadDataBits(&currentData, (u8)lengthExtra);
                        length += (u16)extraBitsValue;
                    }
                    
                    u32 distCode = 0;
                    success = DecodeHuffmanSymbol(&currentData, distHuffTree, decodedCodeLengths + HLIT, HDIST, &distCode);
                    if (!success) {
                        MY_PRINTF("DECODE HUFFMAN FAILED FOR LENGTH CODE");
                        CRASH;
                    }
                    Assert(distCode < CODE_DIST_ALPHA_SIZE-2);
                    u16 distExtra = deflate_distance_codes[distCode][0];
                    u32 dist = deflate_distance_codes[distCode][1];
                    if (distExtra > 0) {
                        Assert(distExtra <= 13);
                        u32 extraDistValue = ReadDataBits(&currentData, (u8)distExtra);
                        dist += extraDistValue;
                    }
                    
                    Assert(dist <= uncompressedSize);
                    u8* startCopy = (uncompressed + uncompressedSize) - dist;
                    for (u32 copyIndex = 0; copyIndex < length; copyIndex++) {
                        *(uncompressed + uncompressedSize++) = *(startCopy + copyIndex);
                    }
                }
            }
            
        }
        else {
            MY_PRINTF("invalid btype of %u", BTYPE);
            CRASH;
        }
        
        if (BFINAL == 1) {
            break;
        }
        blockCount++;
    }
    return uncompressedSize;
}




u32 deflate_decompress(u8* compressed, u32 compressed_len, u8* uncompressed) {
    Deflate_Block_Data deflate_block_data = {};
    deflate_block_data.data = compressed;
    deflate_block_data.sizeBytes = compressed_len;
    deflate_block_data.byteReading = *compressed;
    deflate_block_data.first = &deflate_block_data;
    u32 result = deflate_decompress(&deflate_block_data, uncompressed);
    return result;
}