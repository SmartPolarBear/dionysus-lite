#pragma once

#include "system/types.h"

#include "fs/fs.hpp"

#include "drivers/pci/pci_device.hpp"
#include "drivers/ahci/ahci.hpp"
#include "drivers/ahci/identify_device.hpp"

namespace ahci
{
	error_code ata_port_identify_device(ahci_port* port);

	class ATABlockDevice :
		public file_system::IDevice,
		public file_system::IMemmap
	{
	 public:
		explicit ATABlockDevice(ahci_port* port);

		~ATABlockDevice() override;

		size_t read(void* buf, uintptr_t offset, size_t count) override;
		size_t write(const void* buf, uintptr_t offset, size_t count) override;
		error_code ioctl(size_t req, void* args) override;
		error_code mmap(uintptr_t base, size_t page_count, int prot, size_t flags) override;
	};

}