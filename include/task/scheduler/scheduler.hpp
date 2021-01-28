#pragma once

#include "system/types.h"

#include "debug/thread_annotations.hpp"

#include "task/thread/thread.hpp"
#include "task/scheduler/round_rubin.hpp"

#include "ktl/string_view.hpp"
#include "ktl/list.hpp"

#include "kbl/data/list.hpp"

namespace task
{

struct scheduler_timer
{
	size_t expires{ 0 };
	thread* owner{ nullptr };

	kbl::list_link<scheduler_timer, lock::spinlock> link{ this };
};

class scheduler
{
 public:
	using scheduler_class_type = round_rubin_scheduler_class;
	using timer_list_type = kbl::intrusive_list<scheduler_timer, lock::spinlock, &scheduler_timer::link, true, false>;

	friend class thread;

 public:

	static size_t called_count;

	scheduler() = delete;

	scheduler(const scheduler&) = delete;
	scheduler& operator=(const scheduler&) = delete;

	explicit scheduler(cpu_struct* cpu)
		: scheduler_class(this),
		  owner_cpu(cpu)
	{
		called_count++;
	}

	void schedule() TA_REQ(global_thread_lock);

	void reschedule() TA_EXCL(global_thread_lock);

	void yield() TA_EXCL(global_thread_lock);

	void unblock(thread* t) TA_REQ(global_thread_lock);
	void insert(thread* t) TA_REQ(global_thread_lock);

	void handle_timer_tick() TA_EXCL(global_thread_lock, timer_lock);

 private:

	void enqueue(thread* t) TA_REQ(global_thread_lock);
	void dequeue(thread* t) TA_REQ(global_thread_lock);
	thread* pick_next() TA_REQ(global_thread_lock);
	void timer_tick(thread* t) TA_REQ(global_thread_lock) TA_EXCL(timer_lock);

	timer_list_type timer_list TA_GUARDED(timer_lock) {};

	scheduler_class_type scheduler_class{ this };

	cpu_struct* owner_cpu{ (cpu_struct*)0xdeadbeef };

	mutable lock::spinlock timer_lock{ "scheduler timer" };
};

}