#define DLL_EXPORT
#include "allocator.h"
#include "engine.h"

#define RG_ALLOC_FREE_ON_DESTROY 0
#define RG_ALLOCATOR_DEBUG 0

namespace Engine {

	static std::vector<Allocator*> allocators;
    static STDAllocator*           std_allocator = NULL;

    void RegisterAllocator(Allocator* alloc) {
        allocators.push_back(alloc);
    }

    void FreeAllocator(Allocator* alloc) {
        std::vector<Allocator*>::iterator it = allocators.begin();
        for (; it != allocators.end(); it++) {
            if (*it == alloc) {
                allocators.erase(it);
                return;
            }
        }

        rgLogError(RG_LOG_SYSTEM, "Allocator %s [%p] is not registered!", alloc->GetName(), alloc);
    }

    Allocator* GetAllocator(Uint32 idx) {
        return allocators[idx];
    }

    Uint32 GetRegisteredAllocators() {
        return (Uint32)allocators.size();
    }

    size_t GetTotalAllocatedMemory() {
        size_t len = 0;
        for (size_t i = 0; i < allocators.size(); i++) {
            len += allocators[i]->GetAllocatedMemory();
        }
        return len;
    }

    Allocator::Allocator(String name) {
#if RG_ALLOCATOR_DEBUG
        rgLogWarn(RG_LOG_SYSTEM, "Created allocator: %s", name);
#endif
        this->mutex = SDL_CreateMutex();
        SDL_strlcpy(this->name, name, 32);
        RegisterAllocator(this);
    }

    Allocator::~Allocator() {
        SDL_DestroyMutex(this->mutex);
        FreeAllocator(this);
#if RG_ALLOCATOR_DEBUG
        rgLogWarn(RG_LOG_SYSTEM, "FreeAllocator %s [%p]", GetName(), this);
#endif
    }

    STDAllocator::~STDAllocator() {
#if RG_ALLOC_FREE_ON_DESTROY
        STDBlock b;
        for (Uint32 i = 0; i < blocks.size(); i++) {
            b = blocks[i];
            free(b.ptr);
        }
#else
        if (blocks.size() != 0) {
            rgLogError(RG_LOG_SYSTEM, "%s => Validation failed: Free all allocated blocks by calling 'Deallocate' function!", name);
//            for (Uint32 i = 0; i < blocks.size(); i++) {
//                rgLogError(RG_LOG_SYSTEM, "%s => Allocated block[%db] at: 0x%x", name, blocks[i].len, blocks[i].ptr);
//            }
        }

#if RG_ALLOCATOR_DEBUG
        STDBlock* b;
        for (Uint32 i = 0; i < blocks.size(); i++) {
            b = &blocks[i];
            rgLogError(RG_LOG_SYSTEM, "+-> Block at: %p, len: %ld", b->ptr, b->len);
        }
#endif
#endif

        total = 0;
        blocks.clear();
    }

    void* STDAllocator::Allocate(size_t len) {
        STDBlock b;
        b.len = len;

        if (SDL_LockMutex(mutex) == 0) {
            b.ptr = rg_malloc(len);
            blocks.push_back(b);
        }
        SDL_UnlockMutex(mutex);

        total += len;
#if RG_ALLOCATOR_DEBUG
        rgLogWarn(RG_LOG_SYSTEM, "ALLOC [%s] -> Block at: %p, len: %ld", this->name, b.ptr, b.len);
#endif
        return b.ptr;
    }

    void STDAllocator::Deallocate(void* ptr) {

        if (SDL_LockMutex(mutex) == 0) {
            for (std::vector<STDBlock>::iterator it = blocks.begin(); it != blocks.end(); ++it) {
                if (it->ptr == ptr) {
                    total -= it->len;
                    rg_free(it->ptr);
#if RG_ALLOCATOR_DEBUG
                    rgLogWarn(RG_LOG_SYSTEM, "FREE  [%s] -> Block at: %p, len: %ld", this->name, it->ptr, it->len);
#endif
                    blocks.erase(it);
                    return;
                }
            }
        }
        SDL_UnlockMutex(mutex);

        rgLogError(RG_LOG_SYSTEM, "Error in: %s allocator!", name);
        RG_ERROR_MSG("STD_ALLOCATOR: Invalid pointer!");
    }


    LinearAllocator::LinearAllocator(String name, size_t heapSize) : Allocator(name) {
        this->m_base = rg_malloc(heapSize);
        this->m_cur  = this->m_base;
        this->m_len  = heapSize;
    }

    LinearAllocator::~LinearAllocator() {
        if (this->m_cur != this->m_base) {
            rgLogError(RG_LOG_SYSTEM, "%s => Validation failed: Free all allocated blocks by calling 'Deallocate' function!", name);
        }
        rg_free(this->m_base);
    }

    void* LinearAllocator::Allocate(size_t len) {
        if ((size_t)this->m_cur + len > (size_t)this->m_base + this->m_len) { return NULL; }
        void* ptr = this->m_cur;
        size_t s = (size_t)this->m_cur + len;
        this->m_cur = (void*)s;
        return ptr;
    }

    void LinearAllocator::Deallocate(void* ptr) {
        this->m_cur = this->m_base;
    }


    PoolAllocator::PoolAllocator(String name, size_t poolSize, size_t blockSize) : Allocator(name) {
        this->m_poolSize  = poolSize;
        this->m_blockSize = blockSize;
        this->pool_ptr = rg_malloc(poolSize * blockSize);
        this->DeallocateAll();
    }


    void PoolAllocator::DeallocateAll() {
        this->ul_bc = m_poolSize;
        this->bs    = m_blockSize;
        this->b     = NULL;
        this->ul_b  = pool_ptr;
        this->pool_allocatedBlocks = 0;
    }

    PoolAllocator::~PoolAllocator() {
        if (this->pool_allocatedBlocks != 0) {
            rgLogError(RG_LOG_SYSTEM, "%s => Validation failed: Free all allocated blocks by calling 'Deallocate' function!", name);
        }

        rg_free(this->pool_ptr);
    }

    void* PoolAllocator::Allocate(size_t len) {
        if (SDL_LockMutex(mutex) != 0) { return NULL; }

        if (this->ul_bc > 0) {
            this->ul_bc--;
            void* b = this->ul_b;
            this->ul_b = (void*)(((unsigned char*)this->ul_b) + this->bs);
            this->pool_allocatedBlocks++;
            return b;
        } else if (this->b) {
            void* b = this->b;
            this->b = ((struct PoolBlock*)this->b)->next;
            this->pool_allocatedBlocks++;
            return b;
        }

        SDL_UnlockMutex(mutex);

        RG_ERROR_MSG("POOL_ALLOCATOR: Out of memory!");

        return NULL;
    }

    void PoolAllocator::Deallocate(void* ptr) {
        if (SDL_LockMutex(mutex) != 0) { return; }

        ((struct PoolBlock*)ptr)->next = this->b;
        this->b = ptr;
        this->pool_allocatedBlocks--;

        SDL_UnlockMutex(mutex);
    }

    void SetDefaultAllocator(STDAllocator* alloc) {
        std_allocator = alloc;
    }

    STDAllocator* GetDefaultAllocator() {
        return std_allocator;
    }

}