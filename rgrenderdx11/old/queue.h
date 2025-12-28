#ifndef _QUEUE_H
#define _QUEUE_H

#include <rgtypes.h>

class RQueue {
	public:
		RQueue(Uint32 len);
		~RQueue();

		void Push(void* ptr);
		void Clear();

		void* Next();

		RG_INLINE Uint32 Size()    { return this->m_len; }
		RG_INLINE Uint32 Current() { return this->m_ptr; }
		RG_INLINE void   Reset()   { this->m_ptr = 0;    }

	private:
		void** m_data = NULL;
		Uint32 m_mlen = 0;
		Uint32 m_len  = 0;
		Uint32 m_ptr  = 0;
};

#endif