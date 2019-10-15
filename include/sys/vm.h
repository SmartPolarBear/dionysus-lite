#if !defined(__INCLUDE_SYS_VM_H)
#define __INCLUDE_SYS_VM_H
#include "boot/multiboot2.h"
#include "sys/types.h"

namespace vm
{
// we use 2MB page, so there's in fact nothing as PTE, only PDE
using pde_t = size_t;
using pde_ptr_t = pde_t *;

void freevm(pde_t *pgdir);

void switch_kernelvm(void);
pde_t *setup_kernelvm(void);
void init_kernelvm(void);
} // namespace vm

#endif // __INCLUDE_SYS_VM_H
