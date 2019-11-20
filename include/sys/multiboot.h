#if !defined(__INCLUDE_SYS_MULTIBOOT_H)
#define __INCLUDE_SYS_MULTIBOOT_H

#include "boot/multiboot2.h"
#include "sys/types.h"

namespace multiboot
{
using multiboot_tag_ptr = multiboot_tag *;
constexpr size_t TAGS_COUNT_MAX = 24;

//This can be increase if the required info become more.
constexpr size_t BOOT_INFO_MAX_EXPECTED_SIZE = 4_KB;

void init_mbi(void);
void parse_multiboot_tags(void);

multiboot_tag_ptr aquire_tag(size_t type);
} // namespace multiboot

#endif // __INCLUDE_SYS_MULTIBOOT_H
