#include "syscall.h"
#include "syscall/syscall_args.hpp"

#include "system/mmu.h"
#include "system/syscall.h"

#include "debug/kdebug.h"
#include "drivers/apic/traps.h"

#include "arch/amd64/cpu/cpuid.h"
#include "arch/amd64/cpu/msr.h"
#include "arch/amd64/cpu/regs.h"

#include "task/process/process.hpp"
#include "task/thread/thread.hpp"
#include "task/ipc/message.hpp"

#include "builtin_text_io.hpp"

#include <format>

using namespace task;

error_code sys_ipc_load_message(const syscall_regs* regs)
{
	auto msg = syscall::args_get<task::ipc::message*, 0>(regs);
	if (!VALID_USER_PTR(reinterpret_cast<uintptr_t>(msg)))
	{
		return -ERROR_INVALID;
	}

	lock::lock_guard g{ cur_thread->ipc_state_.lock_ };

	auto tag = msg->get_tag();
	cur_thread->ipc_state_.set_message_tag_locked(&tag);

	cur_thread->ipc_state_.load_mrs_locked(1, msg->get_items_span());

	return ERROR_SUCCESS;
}