#include "queue.h"
#include "rgrenderdx11.h"

RQueue::RQueue(Uint32 len) {
	this->m_data = (void**)RGetAllocator()->Allocate(sizeof(void*) * len);
	this->m_mlen = len;
}

RQueue::~RQueue() {
	RGetAllocator()->Deallocate(this->m_data);
}

void RQueue::Push(void* ptr) {
	if (ptr == NULL) { return; }
	if (this->m_len >= this->m_mlen) { return; }
	m_data[this->m_len] = ptr;
	this->m_len++;
}

void RQueue::Clear() {
	this->m_len = 0;
	this->m_ptr = 0;
}

void* RQueue::Next() {
	if (this->m_ptr >= this->m_len) { return NULL; }
	void* data = this->m_data[this->m_ptr];
	this->m_ptr++;
	return data;
}