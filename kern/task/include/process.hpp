#pragma once

#include "internals/thread.hpp"

#include "arch/amd64/cpu/regs.h"

#include "system/process.h"

#include "task/thread/thread.hpp"

#include "kbl/data/queue.hpp"
#include "kbl/lock/spinlock.h"

#include "ktl/list.hpp"

using lock::spinlock_struct;
using lock::spinlock_acquire;
using lock::spinlock_initialize_lock;
using lock::spinlock_release;

//extern __thread task::process *current;
extern cls_item<task::process*, CLS_PROC_STRUCT_PTR> cur_proc;

struct process_list_struct
{
	lock::spinlock lock{"TaskLock"};

	[[deprecated("use new head")]] list_head active_head;

	size_t proc_count  TA_GUARDED(lock);
	task::process::head_type head TA_GUARDED(lock);
	kbl::queue<task::process*> zombie_queue TA_GUARDED(lock);

};

extern process_list_struct proc_list;

// task.cc
task::process* find_process(pid_type pid);


