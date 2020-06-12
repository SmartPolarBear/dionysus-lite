#include "arch/amd64/x86.h"

#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"
#include "drivers/console/console.h"
#include "drivers/debug/kdebug.h"
#include "drivers/lock/spinlock.h"

#include "system/error.h"
#include "system/mmu.h"
#include "system/pmm.h"
#include "system/segmentation.hpp"
#include "system/vmm.h"

#include "libraries/libkernel/console/builtin_console.hpp"
#include <cstring>

using trap::trap_handle;
using trap::TRAP_NUMBERMAX;

using lock::spinlock;
using lock::spinlock_acquire;
using lock::spinlock_initlock;
using lock::spinlock_release;

constexpr size_t IDT_SIZE = 4_KB;

static error_code default_trap_handle([[maybe_unused]] trap::trap_frame info);

#pragma clang diagnostic push

#pragma clang diagnostic ignored "-Wc99-designator"
#pragma clang diagnostic ignored "-Winitializer-overrides"

struct
{
	spinlock lock;

	trap_handle trap_handles[TRAP_NUMBERMAX] =
		{
			[0 ... TRAP_NUMBERMAX - 1] = trap_handle{
				.handle = default_trap_handle,
				.enable = true
			},
		};
} handle_table;

#pragma clang diagnostic pop

// default handle of trap
static error_code default_trap_handle([[maybe_unused]] trap::trap_frame info)
{
	// TODO: the handle doesn't exist

	write_format("trap %d caused but not handled.\nerror=%d, ip=0x%p, sp=0x%p\n",
		info.trap_num,
		info.err,
		info.rip,
		info.rsp);

	return ERROR_SUCCESS;
}

static error_code spurious_trap_handle([[maybe_unused]] trap::trap_frame info)
{
	return ERROR_SUCCESS;
}

extern "C" void* vectors[]; // generated by gvectors.py, compiled from apic/vectors.S

// defined below. used by trap_entry to futuer process a trap
extern "C" void trap_body(trap::trap_frame info);

static inline void make_gate(uint32_t* idt_head, uint32_t head_offset, void* kva, uint32_t pl, exception_type type)
{
	uintptr_t addr = (uintptr_t)kva;
	head_offset *= 4;
	uint32_t trap = static_cast<decltype(trap)>(type);
	idt_head[head_offset + 0] = (addr & 0xFFFF) | ((SEGMENTSEL_KCODE) << 16);
	idt_head[head_offset + 1] = (addr & 0xFFFF0000) | trap | ((pl & 3) << 13); // P=1 DPL=pl
	idt_head[head_offset + 2] = addr >> 32;
	idt_head[head_offset + 3] = 0;
}

PANIC void trap::init_trap(void)
{
	uint32_t* idt = reinterpret_cast<uint32_t*>(new BLOCK<IDT_SIZE>);
	memset(idt, 0, IDT_SIZE);

	for (size_t i = 0; i < 256; i++)
	{
		make_gate(idt, i, vectors[i], DPL_KERNEL, IT_INTERRUPT);
	}

	make_gate(idt, 64, vectors[64], DPL_USER, IT_TRAP);

	lidt((uintptr_t)idt, IDT_SIZE);

	spinlock_initlock(&handle_table.lock, "traphandles");

	spinlock_acquire(&handle_table.lock);

	handle_table.trap_handles[trap::irq_to_trap_number(IRQ_SPURIOUS)].handle = spurious_trap_handle;
	handle_table.trap_handles[trap::irq_to_trap_number(IRQ_SPURIOUS)].enable = true;

	spinlock_release(&handle_table.lock);
}

// returns the old handle
PANIC trap_handle trap::trap_handle_regsiter(size_t number, trap_handle handle)
{
	spinlock_acquire(&handle_table.lock);

	trap_handle old = trap_handle{ handle_table.trap_handles[number] };
	handle_table.trap_handles[number] = handle;

	spinlock_release(&handle_table.lock);

	return old;
}

PANIC bool trap_handle_enable(size_t number, bool enable)
{
	spinlock_acquire(&handle_table.lock);

	bool old = handle_table.trap_handles[number].enable;
	handle_table.trap_handles[number].enable = enable;

	spinlock_release(&handle_table.lock);

	return old;
}

extern "C" void trap_body(trap::trap_frame info)
{
	// check if the trap number is out of range
	if (info.trap_num >= TRAP_NUMBERMAX || info.trap_num < 0)
	{
		KDEBUG_RICHPANIC("trap number is out of range", "KERNEL PANIC: TRAP", false, "The given trap number is %d",
			info.trap_num);
	}

	// it should be assigned with the defualt handle when initialized
	KDEBUG_ASSERT(handle_table.trap_handles[info.trap_num].handle != nullptr);

	error_code error = ERROR_SUCCESS;

	// call the handle
	if (handle_table.trap_handles[info.trap_num].enable)
	{
		error = handle_table.trap_handles[info.trap_num].handle(info);
	}
	else
	{
		error = default_trap_handle(info);
	}

	// finish the trap handle
	local_apic::write_eoi();

	if (error != ERROR_SUCCESS)
	{
		KDEBUG_RICHPANIC_CODE(error, true, "");
	}
}

