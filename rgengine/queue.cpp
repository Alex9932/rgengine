#include "queue.h"

typedef struct QueueNode {
	struct QueueNode* next;
	void* data;
} QueueNode;

namespace Engine {
	Queue::Queue(Uint32 size) {
		char allocname[32];
		SDL_snprintf(allocname, 32, "Queue %p", (RG_VPTR)this);
		this->m_alloc = RG_NEW(PoolAllocator)(allocname, size, sizeof(QueueNode));
	}

	Queue::~Queue() {
		RG_DELETE(PoolAllocator, this->m_alloc);
	}

	void Queue::Push(void* data) {
		QueueNode* node = (QueueNode*)this->m_alloc->Allocate();
		if (!node) {
			// Out of memory
			return;
		}

		node->data = data;
		node->next = NULL;
		if (this->m_tail == NULL) {
			// Empty queue
			this->m_head = node;
			this->m_tail = node;
		} else {
			this->m_tail->next = node;
			this->m_tail = node;
		}
	}

	void* Queue::Pop() {
		if (this->m_head) {
			QueueNode* node = this->m_head;
			void* data = node->data;
			this->m_head = node->next;
			if (this->m_head == NULL) {
				this->m_tail = NULL;
			}
			this->m_alloc->Deallocate(node);
			return data;
		}

		return NULL;
	}

};