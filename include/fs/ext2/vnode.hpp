#pragma once

#include "system/types.h"
#include "system/error.hpp"

namespace file_system
{
	class ext2_vnode
		: public vnode_base
	{
	 public:
		~ext2_vnode() override = default;
		error_code find(const char* name, vnode_base& ret) override;
		size_t read_dir(const file_object& fd, directory_entry& entry) override;
		error_code open_dir(const file_object& fd) override;
		error_code open(const file_object& fd) override;
		error_code close(const file_object& fd) override;
		error_code create(const char* filename, uid_type uid, gid_type gid, size_t mode) override;
		error_code make_dir(const char* filename, uid_type uid, gid_type gid, size_t mode) override;
		error_code truncate(size_t size) override;
		error_code unlink(vnode_base& vn) override;
		uintptr_t lseek(const file_object& fd, size_t offset, int whence) override;
		error_code stat(file_status& st) override;
		error_code chmod(size_t mode) override;
		error_code chown(uid_type uid, gid_type gid) override;
		error_code read_link(char* buf, size_t lim) override;
		size_t read(const file_object& fd, void* buf, size_t count) override;
		size_t write(const file_object& fd, const void* buf, size_t count) override;
	};
}