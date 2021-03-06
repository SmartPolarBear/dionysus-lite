#include "arch/amd64/cpu/x86.h"
#include "boot/multiboot2.h"

#include "include/ramdisk.hpp"

#include "drivers/acpi/acpi.h"
#include "drivers/acpi/cpu.h"
#include "drivers/acpi/ap.hpp"
#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"
#include "drivers/apic/timer.h"
#include "drivers/console/console.h"
#include "debug/kdebug.h"
#include "debug/kerror.h"
#include "drivers/monitor/monitor.hpp"
#include "drivers/simd/simd.hpp"
#include "drivers/pci/pci.hpp"
#include "drivers/cmos/rtc.hpp"

#include "system/kmalloc.hpp"
#include "system/memlayout.h"
#include "system/multiboot.h"
#include "system/param.h"
#include "system/pmm.h"
#include "system/scheduler.h"
#include "system/syscall.h"
#include "system/kernel_layout.hpp"
#include "system/vmm.h"

#include "object/object_manager.hpp"

#include "task/scheduler/scheduler.hpp"

#include "fs/fs.hpp"

#include "../libs/basic_io/include/builtin_text_io.hpp"

#include <any>

// std::optional is usable unconditionally
// std::any is usable with the pseudo-syscalls
// std::variant is usable with the pseudo-syscalls
// std::span is usable unconditionally

#include "ktl/atomic.hpp"

//extern std::shared_ptr<task::job> root_job;
//
//static inline void run(const char* name)
//{
//
//	uint8_t* bin = nullptr;
//	size_t size = 0;
//
//	auto ret = multiboot::find_module_by_cmdline(name, &size, &bin);
//
//	KDEBUG_ASSERT(ret == ERROR_SUCCESS);
//
//	auto create_ret = task::process::create(name, bin, size, root_job);
//	if (has_error(create_ret))
//	{
//		KDEBUG_GERNERALPANIC_CODE(get_error_code(create_ret));
//	}
//
//	auto proc = get_result(create_ret);
//
//	write_format("[cpu %d]load binary: %s\n", cpu->id, name);
//}
//

error_code init_thread_routine([[maybe_unused]]void* arg)
{
	write_format("Initialization routine of CPU %d\n", cpu->id);
	if (cpu->id == 0)
	{
		init::load_boot_ramdisk();
	}
	return ERROR_SUCCESS;
}

// global entry of the kernel
extern "C" [[noreturn]] void kmain()
{
	// task the multiboot information
	multiboot::init_mbi();

	// initialize physical memory
	pmm::init_pmm();

	// install trap vectors nad handle structures
	trap::init_trap();

	// initialize the console
	console::console_init();

	// initialize object manager
	object::init_object_manager();

	// initialize ACPI
	acpi::init_acpi();

	// initialize local APIC
	apic::local_apic::init_lapic();

	// initialize apic timer
	timer::init_apic_timer();

	// initialize rtc to acquire date and time
	cmos::cmos_rtc_init();

	// initialize I/O APIC
	apic::io_apic::init_ioapic();

	// initialize syscall
	syscall::system_call_init();

	// initialize SIMD like AVX and sse
	simd::enable_simd();

	// initialize PCI and PCIe
	pci::pci_init();

	// initialize the file system
	file_system::fs_init();

	// initialize user task manager
	task::process_init();

	// boot other CPU cores
	ap::init_ap();

	write_format("Codename \"Dionysus\" (built on %s %s) started.\n", __DATE__, __TIME__);

	ap::all_processor_main();

	for (;;);

}

void ap::all_processor_main()
{
	xchg(&cpu->started, 1u);

	// enable timer interrupt
	timer::mask_cpu_local_timer(cpu->id, false);

	KDEBUG_GERNERALPANIC_CODE(task::thread::create_idle());

	if (auto ret = task::thread::create(nullptr, "init", init_thread_routine, nullptr);has_error(ret))
	{
		KDEBUG_GERNERALPANIC_CODE(get_error_code(ret));
	}
	else
	{
		auto init_thread = get_result(ret);
		init_thread->set_flags(init_thread->get_flags() | task::thread::thread_flags::FLAG_INIT);

		lock::lock_guard g{ task::global_thread_lock };
		task::scheduler::current::unblock(init_thread);
	}

	task::scheduler::current::enter();
}
