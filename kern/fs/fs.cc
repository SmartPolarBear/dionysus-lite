#include "arch/amd64/port_io.h"

#include "system/types.h"
#include "system/memlayout.h"

#include "fs/fs.hpp"
#include "fs/vfs/vfs.hpp"
#include "fs/ext2/ext2.hpp"

#include "drivers/pci/pci.hpp"
#include "drivers/pci/pci_device.hpp"
#include "drivers/pci/pci_header.hpp"
#include "drivers/pci/pci_capability.hpp"
#include "drivers/ahci/ahci.hpp"

#include "libkernel/console/builtin_text_io.hpp"

using namespace libkernel;
using namespace file_system;

list_head fs_class_head;
list_head fs_mount_head;

vfs_io_context the_kernel_io_context{ nullptr, 0, 0 };
vfs_io_context* const file_system::kernel_io_context = &the_kernel_io_context;

// workaround the problem that it's impossible to use current
// implementation of intrusive linked list with non-pod types

struct fs_class_node
{
	file_system::fs_class_base* fs_class;

	list_head link;
};

static inline error_code kernel_io_context_init()
{
	auto dev_find_ret = device_find_first(DC_BLOCK, "sda1");
	if (has_error(dev_find_ret))
	{
		return get_error_code(dev_find_ret);
	}

	auto vnode = get_result(dev_find_ret);

	auto err = kernel_io_context->mount("/", vnode->get_dev(), FSC_EXT2, 0, nullptr);

	if (err != ERROR_SUCCESS)
	{
		return err;
	}

	return ERROR_SUCCESS;
}

PANIC void file_system::fs_init()
{
	// Initialize list heads
	list_init(&fs_class_head);
	list_init(&fs_mount_head);

	// register file system classes
	g_ext2fs.register_this();

	// Initialize devfs root to load real hardware
	auto ret = file_system::init_devfs_root();
	if (ret != ERROR_SUCCESS)
	{
		KDEBUG_RICHPANIC_CODE(ret, false, "");
	}

	// Initialize the fs hardware
	ret = ahci::ahci_init();
	if (ret != ERROR_SUCCESS)
	{
		KDEBUG_RICHPANIC_CODE(ret, false, "");
	}

	ret = file_system::vfs_init();
	if (ret != ERROR_SUCCESS)
	{
		KDEBUG_RICHPANIC_CODE(ret, false, "");
	}

	ret = kernel_io_context_init();
	if (ret != ERROR_SUCCESS)
	{
		KDEBUG_RICHPANIC_CODE(ret, false, "");
	}
	
}

error_code_with_result<fs_instance*> file_system::fs_create(fs_class_base* fs_class,
	device_class* dev,
	size_t flags,
	const char* data)
{
	fs_instance* fs_ins = new fs_instance;
	if (fs_ins == nullptr)
	{
		return -ERROR_MEMORY_ALLOC;
	}

	fs_ins->fs_class = fs_class;
	fs_ins->dev = dev;
	fs_ins->flags = flags;

	error_code ret = ERROR_SUCCESS;
	if ((ret = fs_class->initialize(fs_ins, data)) != ERROR_SUCCESS)
	{
		delete fs_ins;
		return ret;
	}

	list_add(&fs_ins->link, &fs_mount_head);

	return fs_ins;
}

error_code file_system::fs_register(fs_class_base* fscls)
{
	if (fs_find(fscls->get_id()) != nullptr)
	{
		return -ERROR_REWRITE;
	}

	fs_class_node* node = new fs_class_node
		{
			.fs_class=fscls,
		};

	list_add(&node->link, &fs_class_head);
	return ERROR_SUCCESS;
}

file_system::fs_class_base* file_system::fs_find(fs_class_id id)
{
	list_head* iter = nullptr;
	list_for(iter, &fs_class_head)
	{
		auto entry = list_entry(iter, fs_class_node, link);
		if (entry->fs_class->get_id() == id)
		{
			return entry->fs_class;
		}
	}
	return nullptr;
}

file_system::fs_class_base* file_system::fs_find(const char* name)
{
	list_head* iter = nullptr;
	list_for(iter, &fs_class_head)
	{
		auto entry = list_entry(iter, fs_class_node, link);
		if (strcmp(entry->fs_class->get_name(), name) == 0)
		{
			return entry->fs_class;
		}
	}
	return nullptr;
}

