#if !defined(__INCLUDE_SYS_CR_H)
#define __INCLUDE_SYS_CR_H

#if !defined(__cplusplus)
#define CR0_PM (1 << 0)
#define CR0_EXTTYPE (1 << 4)
#define CR0_PAGING (1 << 31)

#define BOOTCR0 (CR0_PAGING|CR0_PM|CR0_EXTTYPE)
#else
#endif

#endif // __INCLUDE_SYS_CR_H
