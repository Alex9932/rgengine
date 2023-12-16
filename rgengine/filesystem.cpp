#define _CRT_SECURE_NO_WARNINGS
#define DLL_EXPORT
#include "filesystem.h"

#include <map>
#include <cJSON.h>

#include "allocator.h"
#include "engine.h"
#include "rgmath.h"
#include "rgstring.h"

struct rg_fs_header {
    char magic[4];
    Uint32 files;
};

struct rg_fs_file {
    char name[256];
    Uint32 offset;
    Uint32 length;
};

struct rg_filesystem {
    FILE* file_stream;
    rg_fs_header header;
    rg_fs_file* files;
    Uint32* hash_array;
};

struct _file_fs {
    rg_filesystem* filesystem;
    rg_fs_file file;
};

#define FSMAP std::map<Uint32, rg_filesystem*>

namespace Engine {

    static Allocator* allocator;

    static char SYSTEM_PATH[128];
    static char GAMEDATA_PATH[128];
    static char USERDATA_PATH[128];

    static FSMAP  filesystems;
    static Sint32 fs_id;
    static char   string_buffer[128];

    static void _fs_find(_file_fs* ffs, String file) {
        ffs->filesystem = NULL;
        Uint32 hash = rgCRC32(file, (Uint32)SDL_strlen(file));

        for (std::map<Uint32, rg_filesystem*>::iterator it = filesystems.begin(); it != filesystems.end(); ++it) {
            rg_filesystem* filesystem = it->second;
            for (Uint32 i = 0; i < filesystem->header.files; ++i) {
                //rg_fs_file f = filesystem->files[i];
                //if(Engine::Utils::streql(f.name, file)) {
                //    ffs->file = f;
                //    ffs->filesystem = filesystem;
                //    return;
                //}
                if (hash == filesystem->hash_array[i]) {
                    ffs->file = filesystem->files[i];
                    ffs->filesystem = filesystem;
                    return;
                }
            }
        }
    }

    static Resource* GetResourceInFS(String file) {
        _file_fs ffs;
        _fs_find(&ffs, file);
        if (ffs.filesystem == NULL) { return NULL; }
        Resource* res = (Resource*)allocator->Allocate(sizeof(Resource));
        res->length = ffs.file.length;
        res->data = allocator->Allocate(ffs.file.length);
        fseek(ffs.filesystem->file_stream, ffs.file.offset, SEEK_SET);
        RG_FREAD(res->data, 1, ffs.file.length, ffs.filesystem->file_stream);
        return res;
    }

    static ResourceStream* GetResourceStreamFS(String file) {
        _file_fs ffs;
        _fs_find(&ffs, file);
        if (ffs.filesystem == NULL) { return NULL; }
        ResourceStream* res = (ResourceStream*)allocator->Allocate(sizeof(ResourceStream));
        res->file_length = ffs.file.length;
        res->file_offset = ffs.file.offset;
        res->fs_handle = ffs.filesystem;
        res->handle = ffs.filesystem->file_stream;
        res->offset = 0;
        return res;
    }

    void FS_PathFrom(char* dst, String src, Uint32 len) {
        Uint32 n = rg_strcharate(src, '/');
        Uint32 i = 0;
        for (; i < n; i++) {
            dst[i] = src[i];
        }
        dst[i] = 0;
    }

    void FS_ReplaceSeparators(char* dst, String src) {
        Uint32 len = (Uint32)SDL_strlen(src);
        for (Uint32 i = 0; i < len; i++) {
            if (src[i] != '\\') { dst[i] = src[i]; }
            else { dst[i] = '/'; }
        }
    }

    void Filesystem_Initialize(String fsjson) {
        rgLogInfo(RG_LOG_SYSTEM, "Initializing filesystem...");


        if (fsjson == NULL) {
            RG_ERROR_MSG("No fsjson!");
        }

        allocator = new STDAllocator("FS allocator");
        filesystems.clear();
        fs_id = -1;

        rgLogInfo(RG_LOG_SYSTEM, "Loading: %s", fsjson);
        Resource* r = GetResource(fsjson);
        cJSON* json_root = cJSON_ParseWithLength((String)r->data, r->length);

        // Setup path to resource dirs
        String platform = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(json_root, "platform"));
        String gamedata = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(json_root, "gamedata"));
        String userdata = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(json_root, "userdata"));
        SDL_snprintf(SYSTEM_PATH, 128, "%s", platform);
        SDL_snprintf(GAMEDATA_PATH, 128, "%s", gamedata);
        SDL_snprintf(USERDATA_PATH, 128, "%s", userdata);

        // Mount filesystems
        rgLogInfo(RG_LOG_SYSTEM, "Mounting filesystems...");
        cJSON* array = cJSON_GetObjectItemCaseSensitive(json_root, "filesystem");
        int size = cJSON_GetArraySize(array);
        for (int i = 0; i < size; i++) {
            String fs = cJSON_GetStringValue(cJSON_GetArrayItem(array, i));
            Mount(fs);
        }

        cJSON_Delete(json_root);
        FreeResource(r);
    }

    void Filesystem_Destroy() {
        std::map<Uint32, rg_filesystem*>::iterator it = filesystems.begin();
        for (; it != filesystems.end(); it++) {
            Umount(it->first);
        }

        filesystems.clear();
        delete allocator;
    }

    static Bool CheckHash(Uint32 hash) {
        FSMAP::iterator it = filesystems.begin();
        for (; it != filesystems.end(); it++) {
            rg_filesystem* fs = it->second;
            for (Uint32 i = 0; i < fs->header.files; i++) {
                if (fs->hash_array[i] == hash) {
                    rgLogWarn(RG_LOG_SYSTEM, "Hash collision %s", fs->files[i].name);
                    return true;
                }
            }
        }
        return false;
    }

    // RFS packages
    Sint32 Mount(String file) {
        FILE* f = fopen(file, "rb");
        if (f) {
            rg_filesystem* fs = (rg_filesystem*)allocator->Allocate(sizeof(rg_filesystem));
            fs->file_stream = f;
            RG_FREAD(&fs->header, sizeof(rg_fs_header), 1, fs->file_stream);

            if (fs->header.magic[0] != 'r' || fs->header.magic[1] != 'f' || fs->header.magic[2] != 's') {
                SDL_snprintf(string_buffer, 128, "Invalid header at %s!", file);
                RG_ERROR_MSG(string_buffer);
            }

            fs->files = (rg_fs_file*)allocator->Allocate(sizeof(rg_fs_file) * fs->header.files);
            fs->hash_array = (Uint32*)allocator->Allocate(sizeof(Uint32) * fs->header.files);
            RG_FREAD(fs->files, sizeof(rg_fs_file), fs->header.files, fs->file_stream);
            fs_id++;

            // Generate hash array
            for (Uint32 i = 0; i < fs->header.files; i++) {
                String fstr = fs->files[i].name;
                fs->hash_array[i] = rgCRC32(fstr, (Uint32)SDL_strlen(fstr));
                if (CheckHash(fs->hash_array[i])) {
                    rgLogWarn(RG_LOG_SYSTEM, "Hash collision %s [%d] in %s", fstr, fs->hash_array[i], file);
                }
                if (IsDebug()) {
                    rgLogInfo(RG_LOG_SYSTEM, "File: %s", fstr);
                }
            }

            filesystems[fs_id] = fs;
            rgLogInfo(RG_LOG_SYSTEM, "Mounting %s, %d files mapped. Used memory: %ldKb", file, fs->header.files, (Uint32)allocator->GetAllocatedMemory() / 1000);

            return fs_id;
        }
        return -1;
    }

    void Umount(Sint32 fs_uid) {
        if (fs_uid < 0) {
            rgLogError(RG_LOG_SYSTEM, "Unable to unmount %d filesystem!", fs_uid);
            return;
        }
        rg_filesystem* fs = filesystems[fs_uid];
        fclose(fs->file_stream);
        allocator->Deallocate(fs->files);
        allocator->Deallocate(fs->hash_array);
        allocator->Deallocate(fs);
        filesystems[fs_uid] = NULL;
    }

    // Resource
    Resource* GetResource(String file) {
        RG_ASSERT(file);

        rgLogWarn(RG_LOG_DEBUG, "Open resource: %s", file);

        Resource* res = GetResourceInFS(file);
        if (res != NULL) { return res; }

        FILE* fptr = fopen(file, "rb");
        SDL_snprintf(string_buffer, 128, "FILE NOT FOUND => %s", file);
        RG_ASSERT_MSG(fptr, string_buffer);
        res = (Resource*)allocator->Allocate(sizeof(Resource));
        fseek(fptr, 0, SEEK_END);
        res->length = ftell(fptr);
        rewind(fptr);
        res->data = allocator->Allocate(res->length);
        RG_FREAD(res->data, 1, res->length, fptr);
        fclose(fptr);
        return res;
    }

    void FreeResource(Resource* res) {
        allocator->Deallocate(res->data);
        allocator->Deallocate(res);
    }

    // Resource stream
    ResourceStream* OpenResourceStream(String file) {
        ResourceStream* stream = GetResourceStreamFS(file);
        if (stream != NULL) { return stream; }

        FILE* fptr = fopen(file, "rb");
        sprintf(string_buffer, "FILE NOT FOUND => %s", file);
        RG_ASSERT_MSG(fptr, string_buffer);

        stream = (ResourceStream*)allocator->Allocate(sizeof(ResourceStream));
        fseek(fptr, 0, SEEK_END);
        stream->file_length = ftell(fptr);
        rewind(fptr);
        stream->file_offset = 0;
        stream->fs_handle = NULL;
        stream->handle = fptr;
        stream->offset = 0;

        return stream;
    }

    void CloseResourceStream(ResourceStream* stream) {
        if (stream->fs_handle == NULL) { fclose(stream->handle); }
        allocator->Deallocate(stream);
    }

    size_t ReadResourceStream(void* ptr, size_t length, ResourceStream* stream) {
        Uint32 s_offset = 0;
        if (stream->fs_handle == NULL) { s_offset = stream->offset; }
        else { s_offset = stream->file_offset + stream->offset; }
        fseek(stream->handle, s_offset, SEEK_SET);
        stream->offset += (Uint32)length;
        return fread(ptr, 1, length, stream->handle);
    }

    Bool EofResourceStream(ResourceStream* stream) {
        Bool eof = stream->offset >= stream->file_length;
        Uint32 s_offset = 0;
        if (stream->fs_handle == NULL) { s_offset = stream->offset; }
        else { s_offset = stream->file_offset + stream->offset; }

        // TODO !!! FIX THIS !!!
        fseek(stream->handle, s_offset, SEEK_SET);
        return feof(stream->handle) || eof;
    }

    FSReader::FSReader(String file) : FSInputStream() { this->m_stream = OpenResourceStream(file); }
    FSReader::FSReader(ResourceStream* stream) { this->m_stream = stream; }
    FSReader::~FSReader() { CloseResourceStream(this->m_stream); }
    size_t FSReader::Read(void* ptr, size_t len) {
        this->m_readed_blocks = ReadResourceStream(ptr, len, this->m_stream);
        return this->m_readed_blocks;
    }

    FSWriter::FSWriter(String file) : FSOutputStream() { this->m_handle = fopen(file, "wb"); }
    FSWriter::~FSWriter() { fclose(this->m_handle); }
    void FSWriter::Write(void* ptr, size_t len) { fwrite(ptr, len, 1, this->m_handle); this->m_offset += len; }

    FSMemoryInputStream::FSMemoryInputStream(void* ptr, size_t len) {
        this->m_ptr = ptr;
        this->m_len = len;
        this->m_offset = 0;
    }
    FSMemoryInputStream::~FSMemoryInputStream() {}
    size_t FSMemoryInputStream::Read(void* ptr, size_t len) {
        if (EndOfStream()) { return 0; }

        Uint8* u8_ptr = (Uint8*)this->m_ptr;
        size_t b = SDL_min(this->m_len - this->m_offset, len);
        SDL_memcpy(ptr, &u8_ptr[this->m_offset], b);
        this->m_offset += b;
        return b;
    }

    // Utility
    void GetPath(char* dst, size_t maxlen, PathType type, String path) {
        char* _path = NULL;
        switch (type) {
            case RG_PATH_SYSTEM:   { _path = SYSTEM_PATH; break;   }
            case RG_PATH_GAMEDATA: { _path = GAMEDATA_PATH; break; }
            case RG_PATH_USERDATA: { _path = USERDATA_PATH; break; }
            default: { break; }
        }
        RG_ASSERT_MSG(_path, "Invalid path type!");
        SDL_snprintf(dst, maxlen, "%s/%s", _path, path);
    }
}