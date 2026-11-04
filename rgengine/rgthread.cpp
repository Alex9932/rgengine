#include "rgthread.h"
#include "allocator.h"
#include <vector>
#include <thread>
#include <deque>
#include <mutex>
#include <condition_variable>

#define RG_MAX_WORKERS 32
#define RG_MAX_POOLS   4

namespace Engine {

	struct WorkerPool {
		std::vector<std::thread> workers;
		std::deque<Task>         queue;
		std::mutex               mtx;
		std::condition_variable  cv_work;
		std::condition_variable  cv_idle;
		String                   name;
		Uint32                   pending = 0;
		Bool                     quit = false;
	};

	static Uint32 cores;
	static Uint32 threads;
	static WorkerPool* pools[RG_MAX_POOLS];

	static void RunOne(WorkerPool* p, std::unique_lock<std::mutex>& lk) {
		Task t = p->queue.front();
		p->queue.pop_front();
		lk.unlock();
		t.proc(t.userdata);
		lk.lock();
		if (--p->pending == 0) { p->cv_idle.notify_all(); }
	}

	static void WorkerProc(WorkerPool* p, Uint32 idx) {
		std::unique_lock<std::mutex> lk(p->mtx);
		//rgLogInfo(RG_LOG_SYSTEM, "Starting worker[%s # %d]...", p->name, idx);
		while (1) {
			p->cv_work.wait(lk, [p] { return p->quit || !p->queue.empty(); });
			if (p->queue.empty()) { return; }
			RunOne(p, lk);
		}
		//rgLogInfo(RG_LOG_SYSTEM, "Stopping worker[%s # %d]...", p->name, idx);
	}

	static void CreatePool(String name, Uint32 id, Uint32 workers) {
		WorkerPool* pool = RG_NEW(WorkerPool);
		rgLogInfo(RG_LOG_SYSTEM, "[%s] Starting %d workers", name, workers);
		pool->workers.reserve(workers);
		pool->name = name;
		pool->pending = 0;
		pool->quit = false;
		pools[id] = pool;
		for (Uint32 i = 0; i < workers; i++) {
			pool->workers.emplace_back(WorkerProc, pool, i);
		}
	}

	static void DestroyPool(Uint32 id) {
		WorkerPool* p = pools[id];
		if (!p) { return; }

		{
			std::lock_guard<std::mutex> lock(p->mtx);
			p->quit = true;
		}

		p->cv_work.notify_all();
		rgLogInfo(RG_LOG_SYSTEM, "Waiting threads[%s : %d]...", p->name, id);
		for (std::thread& t : p->workers) { t.join(); }
		pools[id] = NULL;
		RG_DELETE(WorkerPool, p);
	}

	static void WaitPool(Uint32 id) {
		if (id >= RG_MAX_POOLS) { return; }
		WorkerPool* p = pools[id];
		if (!p) { return; }

		std::unique_lock<std::mutex> lk(p->mtx);
		while (p->pending > 0) {
			if (!p->queue.empty()) {
				RunOne(p, lk);
			} else {
				p->cv_idle.wait(lk, [p] { return p->pending == 0 || !p->queue.empty(); });
			}
		}
	}

	Bool ThreadDispatch(Task* task, Uint32 poolid) {
		if (poolid >= RG_MAX_POOLS || !pools[poolid]) { return false; }

		WorkerPool* p = pools[poolid];

		{
			std::lock_guard<std::mutex> lk(p->mtx);
			if (p->quit) { return false; }
			p->queue.push_back(*task);
			p->pending++;
		}

		p->cv_work.notify_one();
		return true;
	}

	void Thread_Initialize(Uint32 tcount) {
		cores = SDL_GetNumLogicalCPUCores(); // TODO: Get physical CPU cores count
		threads = (tcount > RG_MAX_WORKERS) ? RG_MAX_WORKERS : tcount;

		CreatePool("IO pool", RG_TASK_IO, 2);
		CreatePool("Async pool", RG_TASK_ASYNC, threads);
		CreatePool("Frame pool", RG_TASK_FRAME, threads);
	}

	void Thread_Destroy() {
		DestroyPool(RG_TASK_IO);
		DestroyPool(RG_TASK_ASYNC);
		DestroyPool(RG_TASK_FRAME);
	}

	void Thread_Execute() { pools[RG_TASK_FRAME]->cv_work.notify_one(); }

	void Thread_WaitForAll()       { WaitPool(RG_TASK_FRAME); } // Wait PER FRAME tasks
	void Thread_WaitFor(Uint32 id) { WaitPool(id); }            // Wait for a specific pool

	Uint32 GetCPUCores() { return cores; }
}