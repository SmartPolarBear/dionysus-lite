#include "syscall.h"

#include "system/mmu.h"
#include "system/proc.h"
#include "system/syscall.h"

#include "drivers/debug/kdebug.h"
#include "drivers/apic/traps.h"

#include "arch/amd64/cpuid.h"
#include "arch/amd64/msr.h"
#include "arch/amd64/regs.h"

#include "libraries/libkernel/console/builtin_console.hpp"

error_code sys_put_str(const syscall_regs* regs)
{
//	write_format("[cpu%d]", cpu->id);
	char* strbuf = (char*)get_nth_arg(regs, 0);

	put_str(strbuf);

	return ERROR_SUCCESS;
}

error_code sys_put_char(const syscall_regs* regs)
{
	put_char((char)get_nth_arg(regs, 0));

	return ERROR_SUCCESS;
}