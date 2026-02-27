#ifndef _QUEUE_H
#define _QUEUE_H

#include "rgtypes.h"
#include "allocator.h"

struct QueueNode;

namespace Engine {

	class Queue {
		public:
			Queue(Uint32 size);
			~Queue();

			void Push(void* data);
			void* Pop();

		private:
			PoolAllocator* m_alloc = NULL;
			QueueNode*     m_head  = NULL;
			QueueNode*     m_tail  = NULL;

	};

};

#endif