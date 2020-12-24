#pragma once

#include "arch/amd64/regs.h"

#include "system/mmu.h"
#include "system/types.h"
#include "system/vmm.h"
#include "system/syscall.h"
#include "system/messaging.hpp"

#include "drivers/apic/traps.h"
#include "drivers/lock/spinlock.h"

#include "data/pod_list.h"
#include "data/list_base.hpp"

#include "task/task_dispatcher.hpp"
#include "task/process.hpp"
#include "task/job.hpp"
#include "task/thread.hpp"


#include <cstring>
#include <algorithm>
#include <span>

namespace process
{






// Per-process state
//	struct process_dispatcher
//	{
//		static constexpr size_t KERNSTACK_PAGES = 2;
//		static constexpr size_t KERNSTACK_SIZE = KERNSTACK_PAGES * PAGE_SIZE;
//
//		char name[PROC_NAME_LEN + 1];
//
//		process_state state;
//
//		process_id id;
//		process_id parent_id;
//
//		size_t runs;
//
//		uintptr_t kstack;
//
//		vmm::mm_struct* memory;
//
//		size_t flags;
//		size_t wating_state;
//		error_code exit_code;
//
//		trap::trap_frame* tf;
//		context* context;
//
//		struct sleep_data_struct
//		{
//			size_t channel;
//		} sleep_data{};
//
//		struct messaging_data_struct
//		{
//			static constexpr size_t INTERNAL_BUF_SIZE = 64;
//
//			lock::spinlock lock;
//
//			// message passing
//			void* data;
//			size_t data_size;
//			uint8_t internal_buf[INTERNAL_BUF_SIZE];
//
//			// page passing
//			bool can_receive;
//			void* dst;
//			size_t unique_value;
//			process_id from;
//			uint64_t perms;
//
//		} messaging_data{};
//
//		list_head link;
//
//		process_dispatcher(const char* name, process_id id, process_id parent_id, size_t flags)
//			: state(PROC_STATE_EMBRYO), id(id), parent_id(parent_id),
//			  runs(0), kstack(0), memory(nullptr), flags(flags), wating_state(PROC_WAITING_NONE),
//			  exit_code(ERROR_SUCCESS), tf(nullptr), context(nullptr)
//		{
//			memmove(this->name, name, std::min((size_t)strlen(name), PROC_NAME_LEN));
//
//			lock::spinlock_initialize_lock(&messaging_data.lock, name);
//		}
//	};

/// the beginning of new process. Barely return to user_proc_entry
	void new_proc_begin();

	void process_init(void);

// create a process
	error_code create_process(IN const char* name,
		IN size_t flags,
		IN bool inherit_parent,
		OUT process_dispatcher** ret);

	error_code process_load_binary(IN process_dispatcher* porc,
		IN uint8_t* bin,
		IN size_t binary_size,
		IN binary_types type,
		IN size_t flags);

	// handle process cleanup when exiting
	void process_exit(IN process_dispatcher* proc);

	// terminate current process
	error_code process_terminate(error_code error_code);

	// sleep on certain channel
	error_code process_sleep(size_t channel, lock::spinlock* lock);

	// wake up processes sleeping on certain channel
	error_code process_wakeup(size_t channel);
	error_code process_wakeup_nolock(size_t channel);

	// send and receive message
	error_code process_ipc_send(process_id pid, IN const void* message, size_t size);
	error_code process_ipc_receive(OUT void* message_out);
	// send and receive a page
	error_code process_ipc_send_page(process_id pid, uint64_t unique_val, IN const void* page, size_t perm);
	error_code process_ipc_receive_page(OUT void* out_page);

	// allocate more memory
	error_code process_heap_change_size(IN process_dispatcher* proc, IN OUT uintptr_t* heap_ptr);

} // namespace process

//extern __thread process::process_dispatcher* current;
extern CLSItem<process::process_dispatcher*, CLS_PROC_STRUCT_PTR> cur_proc;