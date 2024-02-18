/*
 * rgEngine utf8.h
 *
 *  Created on: Apr 12, 2022
 *      Author: alex9932
 */
#ifndef _UTF8_H
#define _UTF8_H

#include "rgtypes.h"

#define RG_UTF8_BUFFER_LENGTH 4096

class UTF8Decoder {
    private:
        Uint32 len = 0;
        Uint16 buffer[RG_UTF8_BUFFER_LENGTH];

    public:
        RG_INLINE UTF8Decoder() { SDL_memset(this->buffer, 0, RG_UTF8_BUFFER_LENGTH); }
        RG_INLINE virtual ~UTF8Decoder() {}
        RG_DECLSPEC void DecodeString(String str);
        RG_DECLSPEC Uint32 CharAt(String str, Uint32 char_pos);
        RG_INLINE Uint16* GetResult() { return buffer; }
        RG_INLINE Uint32 GetResultLength() { return len; }
};


class UTF8Encoder {
    private:
        char buffer[RG_UTF8_BUFFER_LENGTH];

    public:
        UTF8Encoder() { SDL_memset(this->buffer, 0, RG_UTF8_BUFFER_LENGTH); }
        virtual ~UTF8Encoder() {}
        RG_DECLSPEC void EncodeString(const Uint16* str);
        RG_INLINE String GetResult() { return buffer; }
};

RG_DECLSPEC void UTF8_FromSJIS(String sjis);
RG_DECLSPEC void UTF8_FromUTF16(WString utf16, Uint32 len);
RG_DECLSPEC String UTF8_GetBuffer();

#endif