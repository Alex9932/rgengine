#ifndef _RGTHREAD_H
#define _RGTHREAD_H

#include "rgtypes.h"

typedef void (*PFN_TASKPROC)(void* userdata);

typedef struct Task {
	PFN_TASKPROC proc;
	void*        userdata;
	//Bool         isDone;
} Task;

namespace Engine {

	// Public
	RG_DECLSPEC Uint32 GetCPUCores();
	// RG_DECLSPEC Uint32 GetThreads(); // Defined in engine.h

	// Return FALSE if task can not be dispatched
	RG_DECLSPEC Bool ThreadDispatch(Task* task);


	// DLL-only
	void Thread_Initialize(Uint32 tcount);
	void Thread_Destroy();

	void Thread_Execute();
	void Thread_WaitForAll();

}

#endif