#ifndef _STACK_H
#define _STACK_H

#include <rgtypes.h>
#include <allocator.h>

class Stack {
	public:
		// size - element size, length - elements count
		Stack(Engine::Allocator* alloc, Uint32 size, Uint32 length);
		~Stack();

		void   Push(void* data);
		void*  Pop();
		void   Reset();
		Uint32 GetSize();
		void*  Get(Uint32 i);

	private:
		Engine::Allocator* m_alloc; // Allocator
		void*			   m_ptr;   // Stack's pointer
		Uint32             m_top;   // Stack's top
		Uint32             m_size;  // Element size

};

#endif