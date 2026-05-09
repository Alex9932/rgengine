#include "rgthread.h"
#include "allocator.h"

#include <vector>
#include <queue>
#include <thread>
#include <mutex>

#define RG_MAX_WORKERS 32
#define RG_MAX_POOLS   16
#define RG_MAX_TASKS   8192

namespace Engine {

	struct WorkerContext {
		std::thread thr;
		struct WorkerPool* pool;
		Bool isWorking;
	};

	struct WorkerPool {
		PoolAllocator* alloc;
		WorkerContext workers[RG_MAX_WORKERS];
		std::condition_variable condvar;
		std::queue<Task*>       queue;
		std::mutex              lock;
		std::mutex              mtx;
		Uint32                  worker_count;
	};

	static Uint32 cores;
	static Uint32 threads;

	static WorkerPool* pools[RG_MAX_POOLS];

	// fixed-length array
	//static WorkerContext io_workers[RG_MAX_WORKERS];
	//static WorkerContext workers[RG_MAX_WORKERS];
	//static WorkerContext async_workers[RG_MAX_WORKERS];

	//static std::condition_variable     condvar;
	//static std::mutex                  mtx;

	//static std::condition_variable     acondvar;
	//static std::mutex                  amtx;

	static Bool                        t_thrquit = false;

	// Task queue
	//static std::queue<Task*>           t_queue;
	//static std::mutex                  t_lock;
	//static std::queue<Task*>           ta_queue;
	//static std::mutex                  ta_lock;

	//static PoolAllocator*              t_alloc = NULL;
	//static PoolAllocator*              ta_alloc = NULL;

	static Bool RetrieveTask(Task** task, WorkerPool* pool) {
		*task = NULL;
		Bool result = false;
		pool->lock.lock();
		if (!pool->queue.empty()) {
			*task = pool->queue.front();
			pool->queue.pop();
			result = true;
		}
		pool->lock.unlock();
		return result;
	}

	static void WorkerProc(WorkerContext* ctx) {
		WorkerPool* pool = ctx->pool;
		Task* task = NULL;

		for (;;) {
			ctx->isWorking = false;
			{
				std::unique_lock<std::mutex> lock(pool->mtx);
				pool->condvar.wait(lock, [pool] { return t_thrquit || !pool->queue.empty(); });
			}
			if (t_thrquit) { return; }
			ctx->isWorking = true;

			//while (true) {
				Bool hasTask = RetrieveTask(&task, pool);
				//if (!hasTask) { break; }
				if (hasTask && task) {
					task->proc(task->userdata);
					pool->alloc->Deallocate(task);
				//}
			}

		}
	}

	Uint32 GetCPUCores() {
		return cores;
	}

	Bool ThreadDispatch(Task* task, Uint32 poolid) {
		if (poolid >= 3) { return false; }
		Task* t = (Task*)pools[poolid]->alloc->Allocate();
		if (!t) { return false; }
		SDL_memcpy(t, task, sizeof(Task));
		pools[poolid]->lock.lock();
		pools[poolid]->queue.push(t);
		pools[poolid]->lock.unlock();
		pools[poolid]->condvar.notify_one();
		return true;
	}

	static void CreatePool(String poolname, Uint32 id, Uint32 workers) {
		WorkerPool* pool = RG_NEW(WorkerPool);
		pool->worker_count = workers;
		pool->alloc = RG_NEW(PoolAllocator)(poolname, RG_MAX_TASKS, sizeof(Task));

		rgLogInfo(RG_LOG_SYSTEM, "Starting %d workers [%s]", workers, poolname);

		for (Uint32 i = 0; i < workers; i++) {
			pool->workers[i].pool = pool;
			pool->workers[i].thr = std::thread(WorkerProc, &pool->workers[i]);
		}

		pools[id] = pool;
	}

	static void DestroyPool(Uint32 id) {
		pools[id]->condvar.notify_all();

		rgLogInfo(RG_LOG_SYSTEM, "Waiting threads[%d]...", id);
		for (size_t i = 0; i < pools[id]->worker_count; i++) {
			pools[id]->workers[i].thr.join();
		}

		pools[id]->alloc->DeallocateAll();
		RG_DELETE(PoolAllocator, pools[id]->alloc);
		RG_DELETE(WorkerPool, pools[id]);
	}

	void Thread_Initialize(Uint32 tcount) {
		cores = SDL_GetNumLogicalCPUCores(); // TODO: Get physical CPU cores count

		if (tcount > RG_MAX_WORKERS) { threads = RG_MAX_WORKERS; }
		else { threads = tcount; }

		CreatePool("IO pool", RG_TASK_IO, 2);
		CreatePool("Async pool", RG_TASK_ASYNC, threads);
		CreatePool("Frame pool", RG_TASK_FRAME, threads);

	}

	void Thread_Destroy() {
		t_thrquit = true;
		DestroyPool(RG_TASK_IO);
		DestroyPool(RG_TASK_ASYNC);
		DestroyPool(RG_TASK_FRAME);
	}

	void Thread_Execute() {
		pools[RG_TASK_FRAME]->condvar.notify_all();
	}

	static Bool IsThreadsWorking(Uint32 id) {
		for (size_t i = 0; i < pools[id]->worker_count; i++) {
			if (pools[id]->workers[i].isWorking) {
				return true;
			}
		}
		return false;
	}

	void Thread_WaitForAll() {
		for (;;) {
			if (!IsThreadsWorking(RG_TASK_FRAME)) {
				return;
			}
		}
	}

}