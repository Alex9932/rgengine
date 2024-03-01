#include "stack.h"

Stack::Stack(Engine::Allocator* alloc, Uint32 size, Uint32 length) {
	m_alloc = alloc;
	m_ptr = m_alloc->Allocate(size * length);
	m_top = 0;
	m_size = size;
}

Stack::~Stack() {
	m_alloc->Deallocate(m_ptr);
}

void Stack::Push(void* data) {
	Uint8* sp = (Uint8*)m_ptr;
	SDL_memcpy(&sp[m_top], data, m_size);
	m_top += m_size;
}

void* Stack::Pop() {
	if (m_top == 0) { return NULL; }
	m_top -= m_size;
	Uint8* sp = (Uint8*)m_ptr;
	return &sp[m_top];
}

void Stack::Reset() {
	m_top = 0;
}

Uint32 Stack::GetSize() {
	return m_top / m_size;
}

void* Stack::Get(Uint32 i) {
	Uint8* sp = (Uint8*)m_ptr;
	return &sp[i];
}