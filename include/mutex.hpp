#pragma once

#include <mutex>
#include <thread>

typedef void (*die_handler_func)(int);

struct MutexStore {

	public:

		std::mutex podman, podman_sched, podman_sock;

		MutexStore(die_handler_func die_handler);
};

extern MutexStore mutex;
