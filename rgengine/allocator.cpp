#define DLL_EXPORT

#include "allocator.h"
#include "engine.h"

#define RG_ALLOC_FREE_ON_DESTROY 0
#define RG_ALLOCATOR_DEBUG 0

void* rg_alloca(size_t size) {
    void* ptr = NULL;

#ifdef RG_PLATFORM_WINDOWS

    __try {
        ptr = alloca(size);
    }
    __except (GetExceptionCode() == STATUS_STACK_OVERFLOW) {
        RG_ERROR_MSG("STACK OVERFLOW!");
    }

#else

    // TODO: Add exception handler
    ptr = alloca(size);

#endif // RG_PLATFORM_WINDOWS

    if (ptr) { return ptr; }
    RG_ERROR_MSG("OUT OF MEMORY!");
    return NULL;
}

void* rg_malloc(size_t size) {
    if (size > 0x7FFFFFFF) {
        RG_ERROR_MSG("OUT OF MEMORY!");
        return NULL;
    }

    void* ptr = malloc(size);
    if (ptr) { return ptr; }
    RG_ERROR_MSG("OUT OF MEMORY!");
    return NULL;
}

void rg_free(void* ptr) {
    free(ptr);
}

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

        rgLogError(RG_LOG_MEMORY, "Allocator %s [%p] is not registered!", alloc->GetName(), alloc);
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
        rgLogWarn(RG_LOG_MEMORY, "Created allocator: %s", name);
#endif
        SDL_strlcpy(this->name, name, 32);
        RegisterAllocator(this);
    }

    Allocator::~Allocator() {
        FreeAllocator(this);
#if RG_ALLOCATOR_DEBUG
        rgLogWarn(RG_LOG_MEMORY, "FreeAllocator %s [%p]", GetName(), this);
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
            rgLogError(RG_LOG_MEMORY, "%s => Validation failed: Free all allocated blocks by calling 'Deallocate' function!", name);
//            for (Uint32 i = 0; i < blocks.size(); i++) {
//                rgLogError(RG_LOG_MEMORY, "%s => Allocated block[%db] at: 0x%x", name, blocks[i].len, blocks[i].ptr);
//            }
        }

#if RG_ALLOCATOR_DEBUG
        STDBlock* b;
        for (Uint32 i = 0; i < blocks.size(); i++) {
            b = &blocks[i];
            rgLogError(RG_LOG_MEMORY, "+-> Block at: %p, len: %ld", b->ptr, b->len);
        }
#endif
#endif

        total = 0;
        blocks.clear();
    }

    void* STDAllocator::Allocate(size_t len) {
        STDBlock b;
        b.len = len;

        mutex.lock();
        b.ptr = rg_malloc(len);
        blocks.push_back(b);
        mutex.unlock();

        total += len;
#if RG_ALLOCATOR_DEBUG
        rgLogWarn(RG_LOG_MEMORY, "ALLOC [%s] -> Block at: %p, len: %ld", this->name, b.ptr, b.len);
#endif
        return b.ptr;
    }

    void STDAllocator::Deallocate(void* ptr) {

        mutex.lock();
        
        for (std::vector<STDBlock>::iterator it = blocks.begin(); it != blocks.end(); ++it) {
            if (it->ptr == ptr) {
                total -= it->len;
                rg_free(it->ptr);
#if RG_ALLOCATOR_DEBUG
                rgLogWarn(RG_LOG_MEMORY, "FREE  [%s] -> Block at: %p, len: %ld", this->name, it->ptr, it->len);
#endif
                blocks.erase(it);
                mutex.unlock();
                return;
            }
        }
        mutex.unlock();

        rgLogError(RG_LOG_MEMORY, "Error in: %s allocator!", name);
        RG_ERROR_MSG("STD_ALLOCATOR: Invalid pointer!");
    }


    LinearAllocator::LinearAllocator(String name, size_t heapSize) : Allocator(name) {
        this->m_base = rg_malloc(heapSize);
        this->m_cur  = this->m_base;
        this->m_len  = heapSize;
    }

    LinearAllocator::~LinearAllocator() {
        if (this->m_cur != this->m_base) {
            rgLogError(RG_LOG_MEMORY, "%s => Validation failed: Free all allocated blocks by calling 'Deallocate' function!", name);
        }
        rg_free(this->m_base);
    }

    void* LinearAllocator::Allocate(size_t len) {

        mutex.lock();
        if ((size_t)this->m_cur + len > (size_t)this->m_base + this->m_len) { mutex.unlock(); return NULL; }
        void* ptr = this->m_cur;
        size_t s = (size_t)this->m_cur + len;
        this->m_cur = (void*)s;
        mutex.unlock();

        return ptr;
    }

    void LinearAllocator::Deallocate(void* ptr) {
        this->m_cur = this->m_base;
    }


    PoolAllocator::PoolAllocator(String name, size_t poolSize, size_t blockSize) : Allocator(name) {
        this->m_poolSize  = poolSize;
        this->m_blockSize = blockSize;
        this->pool_ptr    = rg_malloc(poolSize * blockSize);
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
            rgLogError(RG_LOG_MEMORY, "%s => Validation failed: Free all allocated blocks by calling 'Deallocate' function!", name);
        }

        rg_free(this->pool_ptr);
    }

    void* PoolAllocator::Allocate(size_t len) {

        mutex.lock();
        if (this->ul_bc > 0) {
            this->ul_bc--;
            void* b = this->ul_b;
            this->ul_b = (void*)(((unsigned char*)this->ul_b) + this->bs);
            this->pool_allocatedBlocks++;

            mutex.unlock();
            return b;
        } else if (this->b) {
            void* b = this->b;
            this->b = ((struct PoolBlock*)this->b)->next;
            this->pool_allocatedBlocks++;

            mutex.unlock();
            return b;
        }

        mutex.unlock();

        RG_ERROR_MSG("POOL_ALLOCATOR: Out of memory!");

        return NULL;
    }

    void PoolAllocator::Deallocate(void* ptr) {
        mutex.lock();
        ((struct PoolBlock*)ptr)->next = this->b;
        this->b = ptr;
        this->pool_allocatedBlocks--;
        mutex.unlock();
    }

    void SetDefaultAllocator(STDAllocator* alloc) {
        std_allocator = alloc;
    }

    STDAllocator* GetDefaultAllocator() {
        return std_allocator;
    }

}