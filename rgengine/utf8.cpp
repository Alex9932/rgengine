#define DLL_EXPORT
#include "utf8.h"
#include <iconv.h>

#define RG_UTF8_1BYTE 0x80
#define RG_UTF8_2BYTE 0xC0
#define RG_UTF8_3BYTE 0xE0
#define RG_UTF8_4BYTE 0xF0

#define RG_UTF8_STENCIL 0xFF00

#define RG_UTF8_STENCIL1 0xFF80
#define RG_UTF8_STENCIL2 0xFFC0
#define RG_UTF8_STENCIL3 0xFFE0
#define RG_UTF8_STENCIL4 0xFFF0

#define RG_ENABLE_4BYTE_CODES 0

static char BUFFER[RG_UTF8_BUFFER_LENGTH];

void UTF8Decoder::DecodeString(String str) {
    SDL_memset(buffer, 0, sizeof(Uint16) * RG_UTF8_BUFFER_LENGTH);

    Uint32 k = 0;
    //printf("Str:");
    Uint32 len = (Uint32)SDL_strlen(str);
    for (Uint32 i = 0; i < len; i++) {
        Uint16 code = 0;
        Uint8 c = str[i];
        if ((c >> 7) != 0) { // Multi-byte char

            if ((c & RG_UTF8_4BYTE) == RG_UTF8_4BYTE) { // 4-byte char
                Uint16 c1 = (Uint16)(c ^ RG_UTF8_4BYTE) << 18;
                Uint16 c2 = (Uint16)(str[++i] ^ RG_UTF8_1BYTE) << 12;
                Uint16 c3 = (Uint16)(str[++i] ^ RG_UTF8_1BYTE) << 6;
                Uint16 c4 = (Uint16)(str[++i] ^ RG_UTF8_1BYTE) ^ RG_UTF8_STENCIL;
                code = c4 | c3 | c2 | c1;
                //printf(" (%x-%x-%x-%x)(4:", c1, c2, c3, c4);
            }
            else if ((c & RG_UTF8_3BYTE) == RG_UTF8_3BYTE) { // 3-byte char
                Uint16 c1 = ((Uint16)(c ^ RG_UTF8_3BYTE) ^ RG_UTF8_STENCIL) << 12;
                Uint16 c2 = ((Uint16)(str[++i] ^ RG_UTF8_1BYTE) ^ RG_UTF8_STENCIL) << 6;
                Uint16 c3 = ((Uint16)(str[++i] ^ RG_UTF8_1BYTE) ^ RG_UTF8_STENCIL);
                code = c3 | c2 | c1;
                //printf(" (%x-%x-%x)(3:", c1, c2, c3);
            }
            else if ((c & RG_UTF8_2BYTE) == RG_UTF8_2BYTE) { // 2-byte char
                Uint16 c1 = (Uint16)(c ^ RG_UTF8_2BYTE) << 6;
                Uint16 c2 = (Uint16)(str[++i] ^ RG_UTF8_1BYTE) ^ RG_UTF8_STENCIL;
                code = c1 | c2;
                //printf(" (%x-%x)(2:", c1, c2);
            }
            else {
                //printf(" (%x)(OUTSIDE:", c);
            }

        }
        else { // Single-byte char
            code = c;
            // TODO print error
            //printf(" (1:");
        }

        buffer[k] = code;
        //printf("%x)", buffer[k]);
        k++;
    }

    buffer[k] = 0;
    this->len = k;
    //if(k != 0) {
    //    this->len = k - 1;
    //} else {
    //    this->len = k;
    //}

    //printf("\n");
}

Uint32 UTF8Decoder::CharAt(String str, Uint32 char_pos) {
    Uint32 k = 0;
    Uint32 cp = 0;
    Uint32 len = (Uint32)SDL_strlen(str);
    for (Uint32 i = 0; i < len; ++i) {
        Uint8 c = str[i];
        if (c >> 7 == 1) { // Multi-byte char

            if ((c & RG_UTF8_4BYTE) == RG_UTF8_4BYTE) { // 4-byte char
                k += 4;
                i += 3;
            }
            else if ((c & RG_UTF8_3BYTE) == RG_UTF8_3BYTE) { // 3-byte char
                k += 3;
                i += 2;
            }
            else if ((c & RG_UTF8_2BYTE) == RG_UTF8_2BYTE) { // 2-byte char
                k += 2;
                i += 1;
            }

        }
        else { // Single-byte char
            k++;
        }

        cp++;

        if (cp > char_pos) {
            break;
        }
    }

    return k;
}

static Uint16 Encode(char* str, Uint16 c) {
    Uint32 c_length = 0;

    if (c >= 0 && c < 0x80) { c_length = 1; }
    else if (c >= 0x80 && c < 0x800) { c_length = 2; }
    else if (c >= 0x800 && c < 0x10000) { c_length = 3; }
#if RG_ENABLE_4BYTE_CODES
    else if (c >= 0x10000 && c < 0x110000) { c_length = 4; }
#endif

    switch (c_length) {
        case 1: {
            str[0] = (char)c;
            break;
        }
        case 2: {
            Uint16 b1 = c & 0x003F;
            Uint16 b2 = (c >> 6) & 0x003F;
            str[1] = b1 ^ RG_UTF8_1BYTE;
            str[0] = b2 ^ RG_UTF8_2BYTE;
            break;
        }
        case 3: {
            Uint16 b1 = c & 0x003F;
            Uint16 b2 = (c >> 6) & 0x003F;
            Uint16 b3 = (c >> 12) & 0x003F;
            str[2] = b1 ^ RG_UTF8_1BYTE;
            str[1] = b2 ^ RG_UTF8_1BYTE;
            str[0] = b3 ^ RG_UTF8_3BYTE;
            break;
        }
#if RG_ENABLE_4BYTE_CODES
        case 4: {
            Uint16 b1 = c & 0x003F;
            Uint16 b2 = (c >> 6) & 0x003F;
            Uint16 b3 = (c >> 12) & 0x003F;
            Uint16 b4 = (c >> 18) & 0x003F;
            str[3] = b1 ^ RG_UTF8_1BYTE;
            str[2] = b2 ^ RG_UTF8_1BYTE;
            str[1] = b3 ^ RG_UTF8_1BYTE;
            str[0] = b4 ^ RG_UTF8_4BYTE;
            break;
        }
#endif
        default:
            break;
    }

    return c_length;
}

void UTF8Encoder::EncodeString(const Uint16* str) {
    SDL_memset(buffer, 0, sizeof(char) * RG_UTF8_BUFFER_LENGTH);

    Uint32 i = 0;
    Uint16 ptr = 0;
    Uint16 c = str[i];
    while (c != 0) {
        ptr += Encode(&buffer[ptr], c);
        i++;
        c = str[i];
    }

}

void UTF8_FromSJIS(String sjis) {
    char inbuf[RG_UTF8_BUFFER_LENGTH + 1] = { 0 };
    char outbuf[RG_UTF8_BUFFER_LENGTH + 1] = { 0 };
    char* in = inbuf;
    char* out = outbuf;
    size_t in_size = (size_t)RG_UTF8_BUFFER_LENGTH;
    size_t out_size = (size_t)RG_UTF8_BUFFER_LENGTH;
    iconv_t ic = iconv_open("UTF-8", "SJIS");
    SDL_memcpy(in, sjis, SDL_strlen(sjis));
    iconv(ic, &in, &in_size, &out, &out_size);

    SDL_memset(BUFFER, 0, RG_UTF8_BUFFER_LENGTH);
    SDL_memcpy(BUFFER, outbuf, SDL_strlen(outbuf));
    iconv_close(ic);
}

void UTF8_FromUTF16(WString utf16, Uint32 len) {
    char inbuf[RG_UTF8_BUFFER_LENGTH + 1] = { 0 };
    char outbuf[RG_UTF8_BUFFER_LENGTH + 1] = { 0 };
    char* in = inbuf;
    char* out = outbuf;
    size_t in_size = (size_t)RG_UTF8_BUFFER_LENGTH;
    size_t out_size = (size_t)RG_UTF8_BUFFER_LENGTH;
    iconv_t ic = iconv_open("UTF-8", "UTF-16LE");
    SDL_memcpy(in, utf16, len);
    iconv(ic, &in, &in_size, &out, &out_size);

    SDL_memset(BUFFER, 0, RG_UTF8_BUFFER_LENGTH);
    SDL_memcpy(BUFFER, outbuf, SDL_strlen(outbuf));
    iconv_close(ic);
}

String UTF8_GetBuffer() {
    return BUFFER;
}