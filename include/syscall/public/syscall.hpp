#pragma once

#include "system/types.h"

namespace syscall
{

constexpr size_t SYSCALL_COUNT_MAX = 512;

constexpr size_t SYSCALL_PARAMETER_MAX = 6;

enum SYSCALL_NUMBER
{
	// starts from 1 for the sake of debugging
	SYS_hello = 1,
	SYS_exit,

	SYS_put_str,
	SYS_put_char,
	SYS_send,
	SYS_send_page,
	SYS_receive,
	SYS_receive_page,
	SYS_set_heap_size,

	SYS_ipc_load_message,
	SYS_ipc_send,
	SYS_ipc_receive,
	SYS_ipc_call,
	SYS_ipc_wait,
};

}