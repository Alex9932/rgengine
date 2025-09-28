#include "rgthread.h"
#include "allocator.h"

#include <vector>
#include <queue>
#include <thread>
#include <mutex>

#define RG_MAX_WORKERS 32
#define RG_MAX_TASKS   4096

namespace Engine {

	struct WorkerContext {
		std::thread thr;
		Bool isWorking;
	};

	static Uint32 cores;
	static Uint32 threads;

	//static std::vector<WorkerContext*> workers;

	// fixed-length array
	static WorkerContext workers[RG_MAX_WORKERS];

	static std::condition_variable     condvar;
	static std::mutex                  mtx;

	static Bool                        t_thrquit = false;

	// Task queue
	static std::queue<Task*>           t_queue;
	static std::mutex                  t_lock;

	static PoolAllocator*              t_alloc = NULL;

	static Bool RetrieveTask(Task** task) {
		*task = NULL;
		Bool result = false;
		t_lock.lock();
		if (!t_queue.empty()) {
			*task = t_queue.front();
			t_queue.pop();
			result = true;
		}
		t_lock.unlock();
		return result;
	}

	static void WorkerProc(WorkerContext* ctx) {
		//WorkerContext* ctx = (WorkerContext*)_ctx;
		Task* task = NULL;

		for (;;) {
			ctx->isWorking = false;
			std::unique_lock<std::mutex> lock(mtx);
			condvar.wait(lock);
			if (t_thrquit) { return; }
			ctx->isWorking = true;

			while (true) {
				Bool hasTask = RetrieveTask(&task);
				if (!hasTask) { break; }
				if (task) {
					task->proc(task->userdata);
					//task->isDone = true;
					t_alloc->Deallocate(task);
				}
			}

		}
	}

	Uint32 GetCPUCores() {
		return cores;
	}

	Bool ThreadDispatch(Task* task) {
		Task* t = (Task*)t_alloc->Allocate();
		if (!t) { return false; }
		SDL_memcpy(t, task, sizeof(Task));
		t_lock.lock();
		t_queue.push(t);
		t_lock.unlock();
		condvar.notify_one();
		return true;
	}

	void Thread_Initialize(Uint32 tcount) {
		cores = SDL_GetNumLogicalCPUCores();

		t_alloc = RG_NEW(PoolAllocator)("Task allocator", RG_MAX_TASKS, sizeof(Task));

		if (tcount > RG_MAX_WORKERS) { threads = RG_MAX_WORKERS; }
		else { threads = tcount; }

		rgLogInfo(RG_LOG_SYSTEM, "Starting %d workers", threads);

		for (Uint32 i = 0; i < threads; i++) {
			workers[i].thr = std::thread(WorkerProc, &workers[i]);
		}
	}

	void Thread_Destroy() {
		t_thrquit = true;
		condvar.notify_all();
		rgLogInfo(RG_LOG_SYSTEM, "Waiting threads...");
		for (size_t i = 0; i < threads; i++) {
			workers[i].thr.join();
		}
		t_alloc->DeallocateAll();
		RG_DELETE(PoolAllocator, t_alloc);
	}

	void Thread_Execute() {
		condvar.notify_all();
	}

	Bool IsThreadsWorking() {
		for (size_t i = 0; i < threads; i++) {
			if (workers[i].isWorking) {
				return true;
			}
		}
		return false;
	}

	void Thread_WaitForAll() {
		for (;;) {
			if (!IsThreadsWorking()) {
				return;
			}
		}
	}

}