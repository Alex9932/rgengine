/*
 * rgEngine filesystem.h
 *
 *  Created on: Feb 13, 2022
 *      Author: alex9932
 */

#ifndef _FILESYSTEM_H
#define _FILESYSTEM_H

#include "rgtypes.h"
#include "rgvector.h"
#include <stdio.h>

#define RG_FREAD(ptr, size, count, handle)  if(fread(ptr, size, count, handle) != count) { \
												rgLogError(RG_LOG_SYSTEM, "IO Error!");    \
											}

#define RG_FS_SEEK_SET 0x00000000
#define RG_FS_SEEK_CUR 0x00000001
#define RG_FS_SEEK_END 0x00000002

enum PathType {
    RG_PATH_SYSTEM = 0,
    RG_PATH_GAMEDATA,
    RG_PATH_USERDATA
};

typedef struct Resource {
    Uint32 length;
    void* data;
} Resource;

typedef struct ResourceStream {
    FILE* handle;
    void* fs_handle; // If !!NOT NULL!! - Reading from RFS package
    Uint32 file_length;
    Uint32 file_offset; // Must be 0 if 'fs_handle' field == NULL
    Uint32 offset;
} ResourceStream;

namespace Engine {

    void Filesystem_Initialize(String fsjson);
    void Filesystem_Destroy();

    // Utility
    RG_DECLSPEC void GetPath(char* dst, size_t maxlen, PathType type, String path);
    RG_DECLSPEC void FS_PathFrom(char* dst, String src, Uint32 len);
    RG_DECLSPEC void FS_ReplaceSeparators(char* dst, String src);
    RG_INLINE void FS_ReplaceSeparators(char* dst) { FS_ReplaceSeparators(dst, dst); }


    static void FixPath(char* dst, String str) {
        size_t len = SDL_strlen(str);
        Uint32 j = 0;

        for (size_t k = 0; k < len; k++) {
            if (k != 0 && str[k - 1] == '\\' && str[k] == '\\') { continue; }
            if (str[k] == '\\') {
                dst[j] = '/';
                j++;
                continue;
            }
            dst[j] = str[k];
            j++;
        }
    }

    // RFS packages
    RG_DECLSPEC Sint32 Mount(String file);
    RG_DECLSPEC void Umount(Sint32 fs_uid);

    // Resource
    RG_DECLSPEC Resource* GetResource(String file);
    RG_DECLSPEC void FreeResource(Resource* res);

    // Resource stream
    RG_DECLSPEC ResourceStream* OpenResourceStream(String file);
    RG_DECLSPEC void CloseResourceStream(ResourceStream* res);
    RG_DECLSPEC size_t ReadResourceStream(void* ptr, size_t length, ResourceStream* res);
    RG_DECLSPEC Bool EofResourceStream(ResourceStream* res);
    RG_FORCE_INLINE size_t TellStream(ResourceStream* res) { return res->offset; }
    RG_FORCE_INLINE void SeekStream(Uint32 a, Uint32 flag, ResourceStream* res) {
        switch (flag) {
        case RG_FS_SEEK_SET: { res->offset = a; break; }
        case RG_FS_SEEK_CUR: { res->offset += a; break; }
        case RG_FS_SEEK_END: { break; } // TODO
        default: { break; }
        }
    }

    class RG_DECLSPEC FSInputStream {
        public:
            FSInputStream() {}
            virtual ~FSInputStream() {}
            virtual size_t Read(void* ptr, size_t len) { return 0; } // !!! OVERRIDE THIS METHOD !!!
            RG_FORCE_INLINE Uint8  ReadU8() { Uint8 tmp;  Read(&tmp, sizeof(Uint8));  return tmp; }
            RG_FORCE_INLINE Sint8  ReadS8() { Sint8 tmp;  Read(&tmp, sizeof(Sint8));  return tmp; }
            RG_FORCE_INLINE Uint16 ReadU16() { Uint16 tmp; Read(&tmp, sizeof(Uint16)); return tmp; }
            RG_FORCE_INLINE Uint16 ReadS16() { Sint16 tmp; Read(&tmp, sizeof(Sint16)); return tmp; }
            RG_FORCE_INLINE Uint32 ReadU32() { Uint32 tmp; Read(&tmp, sizeof(Uint32)); return tmp; }
            RG_FORCE_INLINE Uint32 ReadS32() { Sint32 tmp; Read(&tmp, sizeof(Sint32)); return tmp; }
            RG_FORCE_INLINE Uint64 ReadU64() { Uint64 tmp; Read(&tmp, sizeof(Uint64)); return tmp; }
            RG_FORCE_INLINE Sint64 ReadS64() { Sint64 tmp; Read(&tmp, sizeof(Sint64)); return tmp; }
            RG_FORCE_INLINE float  ReadF32() { float tmp;  Read(&tmp, sizeof(float));  return tmp; }
            RG_FORCE_INLINE double ReadF64() { double tmp; Read(&tmp, sizeof(double)); return tmp; }
            RG_FORCE_INLINE void Read2F32(vec2& v) { Read(&v, sizeof(vec2)); }
            RG_FORCE_INLINE void Read3F32(vec3& v) { Read(&v, sizeof(vec3)); }
            RG_FORCE_INLINE void Read4F32(vec4& v) { Read(&v, sizeof(vec4)); }
    };

    class RG_DECLSPEC FSOutputStream {
        public:
            FSOutputStream() {}
            virtual ~FSOutputStream() {}
            virtual void Write(void* ptr, size_t len) {}
            RG_FORCE_INLINE void WriteU8(Uint8 a) { Write(&a, sizeof(Uint8)); }
            RG_FORCE_INLINE void WriteS8(Sint8 a) { Write(&a, sizeof(Sint8)); }
            RG_FORCE_INLINE void WriteU16(Uint16 a) { Write(&a, sizeof(Uint16)); }
            RG_FORCE_INLINE void WriteS16(Sint16 a) { Write(&a, sizeof(Sint16)); }
            RG_FORCE_INLINE void WriteU32(Uint32 a) { Write(&a, sizeof(Uint32)); }
            RG_FORCE_INLINE void WriteS32(Sint32 a) { Write(&a, sizeof(Sint32)); }
            RG_FORCE_INLINE void WriteU64(Uint64 a) { Write(&a, sizeof(Uint64)); }
            RG_FORCE_INLINE void WriteS64(Sint64 a) { Write(&a, sizeof(Sint64)); }
            RG_FORCE_INLINE void WriteF32(float a) { Write(&a, sizeof(float)); }
            RG_FORCE_INLINE void WriteS64(double a) { Write(&a, sizeof(double)); }
            RG_FORCE_INLINE void Write2F32(vec2* a) { Write(a, sizeof(vec2)); }
            RG_FORCE_INLINE void Write3F32(vec3* a) { Write(a, sizeof(vec3)); }
            RG_FORCE_INLINE void Write4F32(vec4* a) { Write(a, sizeof(vec4)); }
    };

    class RG_DECLSPEC FSReader : public FSInputStream {
        private:
            ResourceStream* m_stream = NULL;
            size_t          m_readed_blocks = 0;

        public:
            FSReader(String file);
            FSReader(ResourceStream* stream);
            virtual ~FSReader();
            virtual size_t Read(void* ptr, size_t len);

            RG_FORCE_INLINE size_t GetOffset() { return m_stream->offset; }
            RG_FORCE_INLINE ResourceStream* GetStream() { return m_stream; }
            RG_FORCE_INLINE size_t GetReadedBlocks() { return m_readed_blocks; }
            RG_FORCE_INLINE Bool EndOfStream() { return EofResourceStream(m_stream); }
            RG_FORCE_INLINE size_t Tell() { return TellStream(m_stream); }
            RG_FORCE_INLINE void Seek(Uint32 a, Uint32 flag) { SeekStream(a, flag, m_stream); }
    };

    class RG_DECLSPEC FSWriter : public FSOutputStream {
        private:
            FILE* m_handle = NULL;
            size_t m_offset = 0;

        public:
            FSWriter(String file);
            virtual ~FSWriter();
            virtual void Write(const void* ptr, size_t len);
            RG_FORCE_INLINE size_t GetOffset() { return m_offset; }
            RG_FORCE_INLINE FILE* GetHandle() { return m_handle; }
    };

    class RG_DECLSPEC FSMemoryInputStream : public FSInputStream {
        private:
            void* m_ptr = NULL;
            size_t m_len = 0;
            size_t m_offset = 0;

        public:
            FSMemoryInputStream(void* ptr, size_t len);
            virtual ~FSMemoryInputStream();
            virtual size_t Read(void* ptr, size_t len);

            RG_FORCE_INLINE void* GetDataPointer() { return m_ptr; }
            RG_FORCE_INLINE Bool EndOfStream() { return m_offset >= m_len; }
            RG_FORCE_INLINE size_t Tell() { return m_offset; }

            //RG_FORCE_INLINE size_t GetOffset() { return stream->offset; }
            //RG_FORCE_INLINE ResourceStream* GetStream() { return stream; }
            //RG_FORCE_INLINE size_t GetReadedBlocks() { return readed_blocks; }
            //RG_FORCE_INLINE Bool EndOfStream() { return EofResourceStream(stream); }
            //RG_FORCE_INLINE size_t Tell() { return TellStream(stream); }
            //RG_FORCE_INLINE void Seek(size_t a, Uint32 flag) { SeekStream(a, flag, stream); }
    };

}

#endif