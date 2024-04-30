/*
 * rgEngine core/allocator.cpp
 *
 *  Created on: Apr 12, 2022
 *      Author: alex9932
 */
#ifndef _ALLOCATOR_H
#define _ALLOCATOR_H

#include "rgtypes.h"
#include <new>
#include <vector>
#include <mutex>

#define RG_ALLOC_DO_NOT_FREE_ON_DESTROY

#define RG_NEW_CLASS(alloc, c) new(alloc->Allocate(sizeof(c))) c
#define RG_DELETE_CLASS(alloc, c, ptr) ptr->~c(); \
                                       alloc->Deallocate(ptr)

#define RG_NEW(c)         RG_NEW_CLASS(Engine::GetDefaultAllocator(), c)
#define RG_DELETE(c, ptr) RG_DELETE_CLASS(Engine::GetDefaultAllocator(), c, ptr)

RG_DECLSPEC void* rg_malloc(size_t size);
RG_DECLSPEC void  rg_free(void* ptr);

typedef struct STDBlock {
    size_t len;
    void* ptr;
} STDBlock;

typedef struct PoolBlock {
    void* next;
} PoolBlock;

namespace Engine {

    class Allocator {
        protected:
            std::mutex mutex;
            char name[32];

        public:
            /*
              Abstract allocator class
              !!! DO NOT MAKE INSTANCES OF THIS CLASS DIRECTLY !!! (Use STDAllocator / PoolAllocator / etc...)
              name - Name for allocator
            */
            RG_DECLSPEC Allocator(String name);
            RG_DECLSPEC virtual ~Allocator();

            RG_INLINE virtual void* Allocate(size_t len) { return NULL; }
            RG_INLINE virtual void Deallocate(void* ptr) {}
            RG_INLINE virtual size_t GetAllocatedMemory() { return 0; }
            RG_INLINE String GetName() { return this->name; }
    };

    // Malloc/Free wrapper
    class STDAllocator : public Allocator {
        protected:
            size_t total = 0;
            std::vector<STDBlock> blocks;

        public:
            /*
              STD memory alloctor (wrapper for malloc/free functions)
              name - Name for allocator
            */
            STDAllocator(String name) : Allocator(name) {}
            RG_DECLSPEC virtual ~STDAllocator();

            RG_DECLSPEC void* Allocate(size_t len);
            RG_DECLSPEC void Deallocate(void* ptr);

            RG_INLINE size_t GetAllocatedMemory() {
                return total;
            }
    };

    // Linear allocator
    class LinearAllocator : public Allocator {
        private:
            void*  m_base;
            void*  m_cur;
            size_t m_len;

        public:
            RG_DECLSPEC LinearAllocator(String name, size_t heapSize);
            RG_DECLSPEC virtual ~LinearAllocator();
            RG_DECLSPEC void* Allocate(size_t len);
            // 'ptr' argument will be ignored. This method free ALL memory.
            RG_DECLSPEC void Deallocate(void* ptr);

            RG_INLINE   void Deallocate() { this->Deallocate(NULL); };
            RG_INLINE size_t GetAllocatedMemory() { return this->m_len; }
            RG_INLINE size_t GetUsedMemory() { return (size_t)this->m_cur - (size_t)this->m_base; }

    };

    // Used code from: https://github.com/adreuter/mempool

    // Pool allocator
    class PoolAllocator : public Allocator {
        private:
            void*  pool_ptr;
            size_t pool_allocatedBlocks;

            size_t bs;      // the size of a block from the memory pool
            size_t ul_bc;   // the count of unlinked blocks in the memory pool
            void*  b;       // a pointer to the head of the linked list of unused blocks
            void*  ul_b;    // a pointer to the first unlinked block

            size_t m_poolSize;
            size_t m_blockSize;

        public:
            /*
              Pool memory alloctor
              name - Name for allocator
              poolSize - Size of memory pool (in blocks)
              blockSize - size of block in pool (in bytes)
            */
            RG_DECLSPEC PoolAllocator(String name, size_t poolSize, size_t blockSize);
            RG_DECLSPEC virtual ~PoolAllocator();

            // 'len' argument will be ignored!
            RG_DECLSPEC void* Allocate(size_t len);
            RG_DECLSPEC void  Deallocate(void* ptr);
            RG_DECLSPEC void  DeallocateAll();


            // Alias for 'void* Allocate(size_t len)'
            RG_INLINE void* Allocate()       { return Allocate(0); }
            RG_INLINE void* GetBasePointer() { return pool_ptr; }

            RG_INLINE size_t GetAllocatedMemory() { return this->ul_bc * this->bs; }           // Returns TOTAL used memory by pool
            RG_INLINE size_t GetUsedMemory() { return this->pool_allocatedBlocks * this->bs; } // Returns ALLOCATED memory
    };

    RG_DECLSPEC void RegisterAllocator(Allocator* alloc);
    RG_DECLSPEC void FreeAllocator(Allocator* alloc);
    RG_DECLSPEC Allocator* GetAllocator(Uint32 idx);
    RG_DECLSPEC Uint32 GetRegisteredAllocators();
    RG_DECLSPEC size_t GetTotalAllocatedMemory();

    void SetDefaultAllocator(STDAllocator* alloc);
    RG_DECLSPEC STDAllocator* GetDefaultAllocator();

}

#endif