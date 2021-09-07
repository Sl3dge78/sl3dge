#pragma once

/*
Very barebones PNG reader. It might crash if something isn't supported.
API should be pretty self explanatory :

PNG_Image *sLoadImage(const char *path);
void sDestroyImage(PNG_Image *image);

bool sQueryImageSize(const char *path, u32 *w, u32 *h);
bool sLoadImageTo(const char *path, void *dst);

*/

#include <stdio.h>
#include <string.h>

#include "sTypes.h"
#include "sMath.h"

typedef struct {
    u32 width;
    u32 height;
    u8 bpp;
    u8 *pixels;
    u32 size;

} PNG_Image;

typedef enum {
    PNG_UNKNOWN = 0,
    PNG_TYPE_IHDR,
    PNG_TYPE_PLTE,
    PNG_TYPE_IDAT,
    PNG_TYPE_IEND
} PNG_PacketType;

typedef struct {
    u32 length;
    PNG_PacketType type;
    u32 type_u32;
    u8 *data;
    u32 crc;
} PNG_Packet;

typedef struct {
    u32 width;
    u32 height;
    u8 bit_depth;
    u8 color_type;
    u8 compression;
    u8 filter;
    u8 interlaced;
} PNG_IHDR;

typedef struct PNG_DataChunk PNG_DataChunk;

struct PNG_DataChunk {
    u8 *data;
    u32 size;
    PNG_DataChunk *next;
};

typedef struct {
    PNG_DataChunk *first;
    PNG_DataChunk *last;

    void *contents;
    u32 contents_size;

    u32 bits_left;
    u32 bit_buffer;

} PNG_DataStream;

typedef struct {
    u8 cm;
    u8 cinfo;
    u8 fcheck;
    bool fdict;
    u8 flevel;

} PNG_IDAT;

const u32 FIXED_LENGTH_TABLE[] = {3,  4,  5,  6,  7,  8,  9,  10, 11,  13,  15,  17,  19,  23, 27,
                                  31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
const u32 LENGTH_EXTRA_BITS[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2,
                                 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};

const u32 DIST_EXTRA_BITS[] = {0, 0, 0, 0, 1, 1, 2, 2,  3,  3,  4,  4,  5,  5,  6,
                               6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
const u32 FIXED_DISTANCE_TABLE[] = {1,    2,    3,    4,    5,    7,    9,    13,    17,    25,
                                    33,   49,   65,   97,   129,  193,  257,  385,   513,   769,
                                    1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};

internal void StreamFlushBits(PNG_DataStream *stream) {
    stream->bits_left = 0;
    stream->bit_buffer = 0;
}

#define StreamRead(stream, type) (type *)StreamReadSize(stream, sizeof(type))

internal void *StreamReadSize(PNG_DataStream *stream, u32 size) {
    void *result = 0;

    if((stream->contents_size == 0) && stream->first->next) {
        PNG_DataChunk *old = stream->first;
        stream->first = stream->first->next;
        stream->contents_size = stream->first->size;
        stream->contents = stream->first->data;
        sFree(old->data);
        sFree(old);
    }

    if(size <= stream->contents_size) {
        result = stream->contents;
        stream->contents = (u8 *)stream->contents + size;
        stream->contents_size -= size;
    } else {
        ASSERT(0);
        sError("File underflow");
    }

    return result;
}

#define StreamPeek(stream, type) *(type *)StreamPeekSize(stream, typeof(type))

internal void *StreamPeekSize(PNG_DataStream *stream, const u32 size) {
    void *result = 0;
    if(stream->contents_size > size) {
        result = stream->contents;
    } else {
        // FIXME
        sError("Overflow, implement me! :)");
    }
    if(!result) {
        sError("File underflow");
        stream->contents_size = 0;
    }

    return result;
}

internal u32 StreamReadBits(PNG_DataStream *stream, const u8 count) {
    u32 result = 0;

    while(stream->bits_left < count) {
        u32 byte = *StreamRead(stream, u8);
        stream->bit_buffer |= byte << stream->bits_left;
        stream->bits_left += 8;
    }

    if(stream->bits_left >= count) {
        stream->bits_left -= count;
        result |= stream->bit_buffer & ((1 << count) - 1);
        stream->bit_buffer >>= count;
    }

    return result;
}

internal void StreamDestroyBits(PNG_DataStream *stream, const u8 count) {
    stream->bits_left -= count;
    stream->bit_buffer >>= count;
}

internal void StreamAppendChunk(PNG_DataStream *stream, PNG_DataChunk *chunk) {
    if(!stream->first) {
        stream->first = chunk;
        stream->contents = chunk->data;
        stream->contents_size = chunk->size;
    }
    if(stream->last) {
        stream->last->next = chunk;
    }
    stream->last = chunk;
}

internal u32 swap_bits(const u32 in, const u8 bit_size) {
    u32 swapped = 0;
    for(u8 i = 0; i < bit_size; ++i) {
        swapped <<= 1;
        u32 bit = in & (1 << i);
        swapped |= (bit > 0);
    }
    return swapped;
}

internal u32 StreamPeekBits(PNG_DataStream *stream, const u8 size) {
    u32 result = 0;
    while(stream->bits_left <= size) {
        u8 byte = *StreamRead(stream, u8);
        stream->bit_buffer |= byte << stream->bits_left;
        stream->bits_left += 8;
    }
    result |= stream->bit_buffer & ((1 << size) - 1);

    return result;
}

internal u32 StreamPeekBitsSwapped(PNG_DataStream *stream, const u8 size) {
    u32 result = 0;
    while(stream->bits_left <= size) {
        u8 byte = *StreamRead(stream, u8);
        stream->bit_buffer |= byte << stream->bits_left;
        stream->bits_left += 8;
    }
    result |= stream->bit_buffer & ((1 << size) - 1);

    return swap_bits(result, size);
}

// Goes through the input array, and writes the amount of occurences of the lengths at index length.
// If there are three 4 size codes in the input, output[4] will be set to 3
internal void HuffmanGetLengthCounts(const u32 input_size, const u32 *input, u32 *output) {
    for(u32 i = 0; i < input_size; ++i) {
        output[input[i]]++;
    }
}

// Returns the maximum value of the array given
internal u32 HuffmanGetMaxLength(const u32 array_size, const u32 *array) {
    u32 result = 0;
    for(u32 i = 0; i < array_size; ++i) {
        if(result < array[i])
            result = array[i];
    }
    return result;
}

// Generates the first values to use for each code length following the Huffman algorithm
internal void HuffmanCreateFirstLengthValues(const u32 *length_counts,
                                             u32 *first_lengths_values,
                                             const u32 size) {
    u32 code = 0;
    for(u32 i = 1; i < size; ++i) {
        code = (code + (length_counts[i - 1])) << 1;
        first_lengths_values[i] = code;
    }
}

internal void HuffmanPrint(const u32 size, const u32 *huffman) {
    for(u32 i = 0; i < size; i++) {
        sTrace(BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(huffman[i]));
    }
}

internal void HuffmanCompute(const u32 size, const u32 *lengths, u32 *huffman_table) {
    u32 max_length = HuffmanGetMaxLength(size, lengths);

    // We will store here the amount of occurences of length i. ie : length_counts[9] == 8 > there are 8 codes with a length of 9
    u32 *length_counts = sCalloc(max_length + 1, sizeof(u32));

    HuffmanGetLengthCounts(size, lengths, length_counts);
    length_counts[0] = 0;
    // We will store here the next value to assign to a length i. if we want to query the next value for length 4 -> length_values[4].
    u32 *length_values = sCalloc(max_length + 1, sizeof(u32));

    length_values[0] = 0;

    HuffmanCreateFirstLengthValues(length_counts, length_values, max_length + 1);

    for(u32 i = 0; i < size; ++i) {
        if(lengths[i] != 0) {
            huffman_table[i] = swap_bits(length_values[lengths[i]]++, lengths[i]);
        }
    }
    sFree(length_values);
    sFree(length_counts);
}

internal u32 HuffmanDecode(PNG_DataStream *stream,
                           const u32 *codes,
                           const u32 *lengths,
                           const u32 size) {
    u32 code = StreamPeekBits(stream, 16);
    for(u32 i = 0; i < size; i++) {
        if(lengths[i] == 0)
            continue;

        u32 mask = (1 << lengths[i]) - 1;
        if((code & mask) == codes[i]) {
            StreamDestroyBits(stream, lengths[i]);
            return i;
        }
    }
    ASSERT_MSG(0, "Unable to find correspondance");

    return 0;
}

internal void PNGPrintHeader(const u8 *header) {
    sTrace(
        "PNG : Header \n      %0.2X\n      %0.2X %0.2X %0.2X\n      %0.2X %0.2X\n      %0.2X\n      %0.2X",
        header[0],
        header[1],
        header[2],
        header[3],
        header[4],
        header[5],
        header[6],
        header[7]);
}

#define PNGHEADER(v1, v2, v3, v4) (((u32)v1 << 24) | ((u32)v2 << 16) | ((u32)v3 << 8) | ((u32)v4))

internal void PNGReadPacket(FILE *file, PNG_Packet *packet) {
    // Length
    fread(&packet->length, sizeof(u32), 1, file);
    packet->length = swap_u32(packet->length);

    // Type
    fread(&packet->type_u32, sizeof(char), 4, file);
    char str[5] = {'\0'};
    memcpy(str, &packet->type_u32, 4);
    packet->type_u32 = swap_u32(packet->type_u32);

    switch(packet->type_u32) {
    case(PNGHEADER('I', 'H', 'D', 'R')): packet->type = PNG_TYPE_IHDR; break;
    case(PNGHEADER('I', 'D', 'A', 'T')): packet->type = PNG_TYPE_IDAT; break;
    case(PNGHEADER('P', 'L', 'T', 'E')): packet->type = PNG_TYPE_PLTE; break;
    case(PNGHEADER('I', 'E', 'N', 'D')): packet->type = PNG_TYPE_IEND; break;
    default: packet->type = PNG_UNKNOWN;
    }

    if(packet->type != PNG_UNKNOWN) {
        sTrace("PNG : Type %s", str);
    } else {
        sWarn("PNG : Type %s (unhandled)", str);
    }

    packet->data = sMalloc(packet->length * sizeof(u8));
    fread(packet->data, sizeof(u8), packet->length, file);

    // CRC
    fread(&packet->crc, sizeof(u32), 1, file);
}

internal bool PNGParse(FILE *file, PNG_DataStream *stream, PNG_Image *image) {
    for(;;) {
        PNG_Packet packet = {0};
        PNGReadPacket(file, &packet);

        if(packet.type == PNG_TYPE_IEND) {
            sFree(packet.data);
            break;
        }

        switch(packet.type) {
        case PNG_TYPE_IHDR: {
            PNG_IHDR hdr = {0};
            memcpy(&hdr, packet.data, sizeof(PNG_IHDR));

            if(hdr.interlaced == 1) {
                sError("Image is interlaced, this isn't supported yet");
                return false;
            }

            if(hdr.color_type != 2 && hdr.color_type != 6) {
                sError("Image isn't RGB or RGBA, this isn't supported yet");
                return false;
            }
            if(hdr.filter != 0) {
                sError("Filter type of 1 found. This isn't standard");
                return false;
            }

            image->width = swap_u32(hdr.width);
            image->height = swap_u32(hdr.height);

            switch(hdr.color_type) {
            //case(0): image->bpp = hdr.bit_depth; break;
            case(2): image->bpp = (hdr.bit_depth * 3) / 8; break;
            //case(3): image->bpp = hdr.bit_depth ; break;
            //case(4): image->bpp = (hdr.bit_depth * 2) / 8; break; // Grayscale + Alpha
            case(6): image->bpp = (hdr.bit_depth * 4) / 8; break; // RGB + Alpha
            }

            image->size = image->width * image->height * image->bpp;

            sTrace("%dx%d (%dbpp)", image->width, image->height, image->bpp);
            sFree(packet.data);
        } break;
        case PNG_TYPE_IDAT: {
            PNG_DataChunk *chunk = sMalloc(sizeof(PNG_DataChunk));
            chunk->next = 0;
            chunk->data = packet.data;
            chunk->size = packet.length;

            StreamAppendChunk(stream, chunk);
        } break;
        default: {
            sTrace("PNG : Skipping packet");
            sFree(packet.data);
            continue;
        }
        }
    }
    return true;
}

internal void PNGDecode(PNG_DataStream *stream, u8 *out_ptr, u8 *dbg_end) {
    // https://www.ietf.org/rfc/rfc1951.txt
    bool bfinal = 0;
    u32 bytes = 0;
    u8 *dbg_start = out_ptr;
    PNG_IDAT idat = {0};
    idat.cm = StreamReadBits(stream, 4);
    idat.cinfo = StreamReadBits(stream, 4);
    idat.fcheck = StreamReadBits(stream, 5);
    idat.fdict = StreamReadBits(stream, 1);
    idat.flevel = StreamReadBits(stream, 2);

    ASSERT(idat.cm == 8);

    if(idat.fdict) {
        sError("ADLER32 in this stream, this isn't handled.");
    }
    do {
        bfinal = StreamReadBits(stream, 1);

        u8 btype = StreamReadBits(stream, 2);
        if(btype == 0) { // Uncompressed
            StreamFlushBits(stream);
            // @TODO
            //u32 len = *StreamRead(stream, u32);
            //u32 nlen = *StreamRead(stream, u32);

            ASSERT_MSG(0, "I am not implemented");
        } else {
            u32 *litlen_table = 0;
            u32 *distance_table = 0;
            u32 *HLITHDISTLengths = 0;
            u32 HDIST = 0;
            u32 HLIT = 0;
            if(btype == 2) { // Dynamic Huffman tree
                HLIT = StreamReadBits(stream, 5) + 257;
                HDIST = StreamReadBits(stream, 5) + 1;
                u32 HCLEN = StreamReadBits(stream, 4) + 4;

                u32 HCLENLengthTable[19] = {0};
                const u32 HCLENSwizzle[] = {
                    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

                for(u32 i = 0; i < HCLEN; i++) {
                    u32 value = StreamReadBits(stream, 3);
                    HCLENLengthTable[HCLENSwizzle[i]] = value;
                }
                u32 *HCLENCodes = sCalloc(19, sizeof(u32));
                HuffmanCompute(19, HCLENLengthTable, HCLENCodes);

                HLITHDISTLengths = sCalloc(HLIT + HDIST, sizeof(u32));
                u32 index = 0;
                while(index < HLIT + HDIST) {
                    u32 decoded = HuffmanDecode(stream, HCLENCodes, HCLENLengthTable, 19);
                    if(decoded < 16) {
                        HLITHDISTLengths[index] = decoded;
                        index++;
                        continue;
                    }

                    u32 repeats = 0;
                    u32 repeated_value;
                    switch(decoded) {
                    case(16): {
                        repeats = StreamReadBits(stream, 2) + 3;
                        repeated_value = HLITHDISTLengths[index - 1];

                    } break;
                    case(17): {
                        repeats = StreamReadBits(stream, 3) + 3;
                        repeated_value = 0;
                    } break;
                    case(18): {
                        repeats = StreamReadBits(stream, 7) + 11;
                        repeated_value = 0;
                    } break;
                    }
                    ASSERT(repeats + index <= HLIT + HDIST);
                    for(u32 i = 0; i < repeats; ++i) {
                        HLITHDISTLengths[index] = repeated_value;
                        index++;
                        ASSERT(index <= HLIT + HDIST);
                    }
                }

                litlen_table = sCalloc(HLIT, sizeof(u32));
                distance_table = sCalloc(HDIST, sizeof(u32));
                HuffmanCompute(HLIT, HLITHDISTLengths, litlen_table);
                HuffmanCompute(HDIST, HLITHDISTLengths + HLIT, distance_table);
                sFree(HCLENCodes);
            } else if(btype == 1) {
                ASSERT(0);
            }
            for(;;) {
                u32 length_code = HuffmanDecode(stream, litlen_table, HLITHDISTLengths, HLIT);

                if(length_code <= 255) {
                    *out_ptr = (u8)(length_code & 0xFF);
                    out_ptr++;
                    bytes++;
                    continue;
                } else if(length_code == 256) {
                    srTrace(".");
                    break;
                } else {
                    u32 length = FIXED_LENGTH_TABLE[length_code - 257];
                    u32 length_extra_bits = LENGTH_EXTRA_BITS[length_code - 257];

                    if(length_extra_bits > 0) {
                        length += StreamReadBits(stream, length_extra_bits);
                    }

                    u32 distance_code =
                        HuffmanDecode(stream, distance_table, HLITHDISTLengths + HLIT, HDIST);
                    ASSERT(distance_code < 30);

                    u32 distance = FIXED_DISTANCE_TABLE[distance_code];
                    u32 dist_extra_bits = DIST_EXTRA_BITS[distance_code];
                    if(dist_extra_bits > 0) {
                        distance += StreamReadBits(stream, dist_extra_bits);
                    }

                    u8 *head = out_ptr - (distance);
                    u32 max_length = dbg_end - dbg_start - bytes;
                    ASSERT(length <= max_length);
                    ASSERT(head >= dbg_start);
                    ASSERT(out_ptr + length <= dbg_end);
                    ASSERT(head + length <= dbg_end);
                    while(length--) {
                        *out_ptr++ = *head++;
                        bytes++;
                    }
                }
            }
            sFree(litlen_table);
            sFree(distance_table);
            sFree(HLITHDISTLengths);
        }
    } while(bfinal == 0);
    srTrace("\n");
    ASSERT(out_ptr == dbg_end);
}

// https://www.w3.org/TR/2003/REC-PNG-20031110/#9Filters
internal void PNGDefilter(PNG_Image *image, u8 *decompressed_image) {
    u8 *line = decompressed_image;
    u32 width_in_bytes = image->width * image->bpp + 1;
    u32 current_line = 0;
    u8 *out_ptr = (u8 *)image->pixels;
    while(current_line < image->height) {
        u8 *in_ptr = line;
        u8 filter = *(in_ptr++);
        switch(filter) {
        case(0): { // No filter
            for(u32 i = 0; i < image->width; i++) {
                out_ptr[0] = in_ptr[0];
                out_ptr[1] = in_ptr[1];
                out_ptr[2] = in_ptr[2];
                out_ptr[3] = image->bpp == 4 ? in_ptr[3] : 0xFF;
                out_ptr += 4;
                in_ptr += image->bpp;
            }

        } break;
        case(1): { // Sub
            u32 a = 0;
            for(u32 i = 0; i < image->width; i++) {
                // Fun times
                out_ptr[0] = in_ptr[0] + ((u8 *)&a)[0];
                out_ptr[1] = in_ptr[1] + ((u8 *)&a)[1];
                out_ptr[2] = in_ptr[2] + ((u8 *)&a)[2];
                out_ptr[3] = image->bpp == 4 ? in_ptr[3] + ((u8 *)&a)[3] : 0xFF;

                a = *(u32 *)out_ptr;
                out_ptr += 4;
                in_ptr += image->bpp;
            }
        } break;
        case(2): { // Up
            u32 b = 0;
            for(u32 i = 0; i < image->width; i++) {
                // Fun times
                out_ptr[0] = in_ptr[0] + ((u8 *)&b)[0];
                out_ptr[1] = in_ptr[1] + ((u8 *)&b)[1];
                out_ptr[2] = in_ptr[2] + ((u8 *)&b)[2];
                out_ptr[3] = image->bpp == 4 ? in_ptr[3] + ((u8 *)&b)[3] : 0xFF;

                b = *(u32 *)(out_ptr - (image->width * 4));
                out_ptr += 4;
                in_ptr += image->bpp;
            }
        } break;
        case(3): { // Avg

            u32 a = 0;
            u32 b = 0;

            for(u32 i = 0; i < image->width; i++) {
                //  @TODO This is horrible
                out_ptr[0] = in_ptr[0] + (((u8 *)&a)[0] + ((u8 *)&b)[0]) / 2;
                out_ptr[1] = in_ptr[1] + (((u8 *)&a)[1] + ((u8 *)&b)[1]) / 2;
                out_ptr[2] = in_ptr[2] + (((u8 *)&a)[2] + ((u8 *)&b)[2]) / 2;
                out_ptr[3] =
                    image->bpp == 4 ? in_ptr[3] + (((u8 *)&a)[3] + ((u8 *)&b)[3]) / 2 : 0xFF;

                a = *(u32 *)out_ptr;
                b = *(u32 *)(out_ptr - (image->width * 4));
                out_ptr += 4;
                in_ptr += image->bpp;
            }
        } break;
        case(4): { // Paeth
            ASSERT_MSG(0, "I am not implemented!");
        } break;
        default: ASSERT_MSG(0, "Unknown filter type encountered");
        }
        current_line++;
        line += width_in_bytes;
    }
}

PNG_Image *sLoadImage(const char *path) {
    PNG_Image *image = sMalloc(sizeof(PNG_Image));
    // Check extension
    u32 length = strlen(path);
    u32 fmt_index = length - 3;
    char extension[4];
    memcpy(extension, path + fmt_index, 3);
    extension[3] = '\0';
    if(strcmp(extension, "png") != 0 && strcmp(extension, "PNG") != 0) {
        sError("Error : Unsupported image format %s", extension);
        return false;
    }

    sTrace("PNG : Begin");
    FILE *file = fopen(path, "rb");
    if(!file) {
        sError("Couldn't open file %s", path);
        return 0;
    }
    u8 header[8];
    fread(header, sizeof(u8), 8, file);

    PNG_DataStream stream = {0};
    bool result = PNGParse(file, &stream, image);
    fclose(file);

    if(!result) {
        sError("Error parsing PNG.");
        sFree(image);
        return 0;
    }

    u32 decompressed_image_size = image->width * image->height * image->bpp + image->height;
    u8 *decompressed_image = sMalloc(decompressed_image_size);
    u8 *decompressed_end = decompressed_image + decompressed_image_size;
    srTrace("PNG : Decoding");
    PNGDecode(&stream, decompressed_image, decompressed_end);
    sTrace("PNG : Decoded");

    for(;;) {
        PNG_DataChunk *old = stream.first;
        if(!old)
            break;
        stream.first = old->next;
        sFree(old->data);
        sFree(old);
    }

    // Defilter
    image->pixels = sMalloc(image->width * image->height * 4); //RGBA always
    sTrace("PNG : Filtering");

    PNGDefilter(image, decompressed_image);

    sTrace("PNG : Filtered");
    image->bpp = 4;
    image->size = image->width * image->height * image->bpp;

    sFree(decompressed_image);

    sTrace("PNG : End");
    return image;
}

// Un peu dégueu peut etre qu'on peut extraire un bout des deux fonctions sans trop s'emmerder?
bool sQueryImageSize(const char *path, u32 *w, u32 *h) {
    FILE *file = fopen(path, "rb");
    ASSERT(file);
    u8 header[8];
    fread(header, sizeof(u8), 8, file);

    bool loop = true;
    while(loop) {
        PNG_Packet packet = {0};
        PNGReadPacket(file, &packet);
        switch(packet.type) {
        case PNG_TYPE_IHDR: {
            PNG_IHDR hdr = {0};
            memcpy(&hdr, packet.data, sizeof(PNG_IHDR));

            if(hdr.interlaced == 1) {
                sError("Image is interlaced, this isn't supported yet");
                return false;
            }

            if(hdr.color_type != 2 && hdr.color_type != 6) {
                sError("Image isn't RGB or RGBA, this isn't supported yet");
                return false;
            }
            if(hdr.filter != 0) {
                sError("Filter type of 1 found. This isn't standard");
                return false;
            }

            *w = swap_u32(hdr.width);
            *h = swap_u32(hdr.height);

            sFree(packet.data);
            loop = false;
        } break;

        default: {
            sFree(packet.data);
            continue;
        }
        }
    }
    fclose(file);
    return true;
}

bool sLoadImageTo(const char *path, void *dst) {
    PNG_Image image = {0};

    // Check extension
    u32 length = strlen(path);
    u32 fmt_index = length - 3;
    char extension[4];
    memcpy(extension, path + fmt_index, 3);
    extension[3] = '\0';
    if(strcmp(extension, "png") != 0 && strcmp(extension, "PNG") != 0) {
        sError("Error : Unsupported image format %s", extension);
        return false;
    }

    sTrace("PNG : Begin");
    FILE *file = fopen(path, "rb");
    ASSERT(file);
    u8 header[8];
    fread(header, sizeof(u8), 8, file);

    PNG_DataStream stream = {0};
    bool result = PNGParse(file, &stream, &image);
    fclose(file);

    if(!result) {
        sError("Error parsing PNG.");
        return 0;
    }

    u32 decompressed_image_size = image.width * image.height * image.bpp + image.height;
    u8 *decompressed_image = sMalloc(decompressed_image_size);
    u8 *decompressed_end = decompressed_image + decompressed_image_size;
    srTrace("PNG : Decoding");
    PNGDecode(&stream, decompressed_image, decompressed_end);
    sTrace("PNG : Decoded");

    for(;;) {
        PNG_DataChunk *old = stream.first;
        if(!old)
            break;
        stream.first = old->next;
        sFree(old->data);
        sFree(old);
    }

    // Defilter
    image.pixels = (u8 *)dst;
    sTrace("PNG : Filtering");
    PNGDefilter(&image, decompressed_image);
    sTrace("PNG : Filtered");
    image.bpp = 4;
    image.size = image.width * image.height * image.bpp;

    sFree(decompressed_image);

    sTrace("PNG : End");
    return true;
}

void sDestroyImage(PNG_Image *image) {
    sFree(image->pixels);
    sFree(image);
}
